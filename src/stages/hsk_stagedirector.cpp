#include "hsk_stagedirector.hpp"
#include <set>

namespace hsk {
    void StageImage::Init(const VkContext* context, VkExtent2D swapchainSize, std::string_view name)
    {
        Image = std::make_unique<ManagedImage>();
        VkExtent3D extent;
        if(Size == ESize::SwapchainExtent)
        {
            extent = VkExtent3D{.width = swapchainSize.width, .height = swapchainSize.height, .depth = 1};
        }
        else
        {
            extent = CustomSize;
        }
        Image->Create(context, VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, extent, UsageFlags, Format, VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED, AspectFlags, name);
    }

    StageDirector& StageDirector::AddStage(RenderStage* stage)
    {
        mStages.push_back(stage);
        return *this;
    }

    void StageDirector::InitOrUpdate(const VkContext* context)
    {
        if(context)
        {
            mContext = context;
        }

        ReorderStages();
        RecreateAndSetImages(mContext->Swapchain.extent);
    }

    void StageDirector::ReorderStages()
    {
        // Lookup for all resources that are already provided
        std::unordered_set<std::string_view> resourceNames;
        // List of all resource references that are already provided
        std::vector<const ResourceReferenceBase*> resourceReferences;
        // List of stages in the order they are gonna be used in
        std::vector<RenderStage*> stageOrder;
        // Lookup of all stages already added
        std::unordered_set<RenderStage*> done;

        // Reorder stages by dependencies

        while(stageOrder.size() != mStages.size())
        {
            bool didInsert = false;
            for(auto stage : mStages)
            {
                if(done.contains(stage))
                {
                    // Skip all stages we already have in the final order
                    continue;
                }

                // Check if all dependencies are provided
                bool hasAll = true;
                for(auto reference : stage->Depends())
                {
                    if(!resourceNames.contains(reference->GetName()))
                    {
                        hasAll = false;
                    }
                }
                if(hasAll)
                {
                    // All dependencies are met, insert into final order
                    stageOrder.push_back(stage);
                    done.emplace(stage);
                    // Log all provided references
                    for(auto reference : stage->Provides())
                    {
                        resourceReferences.push_back(reference);
                        auto insertIter = resourceNames.emplace(reference->GetName());
                        if(!insertIter.second)
                        {
                            // If the insert op returns false, then the resource was already present -> duplicate
                            HSK_THROWFMT("StageDirector: Resource \"{}\" provided by \"{}\" is duplicate! Resource names need to be unique!", reference->GetName(),
                                         stage->GetName())
                        }
                    }
                    didInsert = true;
                    break;
                }
            }
            if(!didInsert)
            {
                // If no stage was inserted, then there is some kind of circular dependency which prevents resolving of the stage graph
                HSK_THROWFMT("StageDirector: Unable to resolve stage order! {} remaining stages.", mStages.size() - stageOrder.size());
            }
        }
        mStages = stageOrder;

        // Build reference bindings list (lookup of all resource reference and which stages use it)

        mReferenceBindings.clear();
        for(const auto reference : resourceReferences)
        {
            // For each reference
            ReferenceBinding binding{.Reference = reference};
            for(int32_t i = 0; i < stageOrder.size(); i++)
            {
                // Check all stages, find out which provides it, and which depend on it, logging that as indices so the required lifetime of the resource can be determined
                RenderStage* stage    = stageOrder[i];
                const auto&  provides = stage->Provides();
                const auto&  depends  = stage->Depends();
                if(std::find(provides.cbegin(), provides.cend(), reference) != provides.cend())
                {
                    // Set the stage index that this resource is provided at
                    binding.ProvidedIndex = i;
                    // We need to provide an output context for this resource, even if it is never read
                    binding.LastUsedIndex = std::max(binding.LastUsedIndex, i);
                    if (binding.SurvivePresent){
                        binding.LastUsedIndex = stageOrder.size() - 1;
                    }
                }
                if(std::find(depends.cbegin(), depends.cend(), reference) != depends.cend())
                {
                    // Set the stage index that this resource is last used at
                    binding.LastUsedIndex = std::max(binding.LastUsedIndex, i);
                }
                const auto imageBinding = dynamic_cast<const StageImageReference*>(reference);
                if(imageBinding)
                {
                    binding.SurvivePresent = imageBinding->ReferenceType == StageImageReference::EReferenceType::InputPreviousFrame;
                    binding.ImageRequirementsHash = imageBinding->ImageInfo.GetRequirementsHash();
                }
            }
            mReferenceBindings.push_back(binding);
        }

        // Detect the count of parallely required images

        std::vector<ImageCountSet> stageCounts(mStages.size());
        for(auto& referenceBinding : mReferenceBindings)
        {
            auto imageRef = dynamic_cast<const StageImageReference*>(referenceBinding.Reference);
            if(imageRef == nullptr)
            {
                // not an image, so not handled by the stage director
                continue;
            }
            for(uint32_t i = referenceBinding.ProvidedIndex; i < referenceBinding.LastUsedIndex; i++)
            {
                auto find = stageCounts[i].find(referenceBinding.ImageRequirementsHash);
                if(find == stageCounts[i].end())
                {
                    stageCounts[i][referenceBinding.ImageRequirementsHash] = ImageResourceCount{.Info = imageRef->ImageInfo, .Count = 1};
                }
                else
                {
                    find->second.Count++;
                }
            }
        }

        // Find the maximum count of parallel images required
        mGlobalCounts.clear();
        for(auto& stageCount : stageCounts)
        {
            for(auto& pair : stageCount)
            {
                auto find = mGlobalCounts.find(pair.first);
                if(find == mGlobalCounts.end())
                {
                    mGlobalCounts[pair.first] = pair.second;
                }
                else
                {
                    find->second.Count = std::max(find->second.Count, pair.second.Count);
                }
            }
        }
    }

    void StageDirector::RecreateAndSetImages(VkExtent2D swapchainSize)
    {
        for(auto& globalCount : mGlobalCounts)
        {
            for(uint32_t i = 0; i < globalCount.second.Count; i++)
            {
                std::unique_ptr<FrameRotator<StageImage, INFLIGHT_FRAME_COUNT>> rotatedImages = std::make_unique<FrameRotator<StageImage, INFLIGHT_FRAME_COUNT>>();
                for(uint32_t f = 0; f < INFLIGHT_FRAME_COUNT; f++)
                {
                    std::string name = fmt::format("StageDirector_{0}.AutoGen.{1:x}.{2}", GetName(), globalCount.first, f);
                    rotatedImages->Get(f).Init(GetContext(), swapchainSize, name);
                }
                mImages.push_back(std::move(rotatedImages));
            }
        }

        std::vector<std::string_view> used(mImages.size());
        for(uint32_t i = 0; i < mStages.size(); i++)
        {
            const auto& depends  = mStages[i]->Depends();
            const auto& provides = mStages[i]->Provides();
        }
    }

    bool StageDirector::Exists() const
    {
        return !!mImages.size();
    }
    void StageDirector::Destroy()
    {
        mImages.clear();
    }
}  // namespace hsk