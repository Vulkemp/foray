#include "hsk_stagedirector.hpp"
#include <set>

namespace hsk {
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
                            HSK_THROWFMT("StageDirector: Resource \"{}\" provided by \"{}\" is duplicate! Resource names need to be unique!", reference->GetName(), stage->GetName())
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

        // Build reference bindings list
        mReferenceBindings.clear();
        for(const auto reference : resourceReferences)
        {
            // For each reference
            ReferenceBinding binding{.Reference = reference};
            for(uint32_t i = 0; i < stageOrder.size(); i++)
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
                }
                if(std::find(depends.cbegin(), depends.cend(), reference) != depends.cend())
                {
                    // Set the stage index that this resource is last used at
                    binding.LastUsedIndex = std::max(binding.LastUsedIndex, i);
                }
                const auto imageBinding = dynamic_cast<const StageImageReference*>(reference);
                if(imageBinding)
                {
                    binding.ImageRequirementsHash = imageBinding->ImageInfo.GetRequirementsHash();
                }
            }
            mReferenceBindings.push_back(binding);
        }

        
    }

    bool StageDirector::Exists() const
    {
        return !!mImages.size();
    }
    void StageDirector::Cleanup()
    {
        mImages.clear();
    }
}  // namespace hsk