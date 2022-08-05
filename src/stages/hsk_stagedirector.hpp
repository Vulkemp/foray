#pragma once
#include "../hsk_basics.hpp"
#include "../utility/hsk_framerotator.hpp"
#include "hsk_renderstage.hpp"
#include "hsk_stagereferences.hpp"
#include <limits>

namespace hsk {

    struct StageImage : public StageImageInfo, public DeviceResourceBase
    {
      public:
        inline StageImage() {}

        inline virtual void Init(const VkContext* context, VkExtent2D size, std::string_view name);

        inline virtual bool Exists() const override { return Image->Exists(); }
        inline virtual void Cleanup() override { Image->Cleanup(); }

        virtual ~StageImage() { Image->Cleanup(); }

        std::unique_ptr<ManagedImage> Image;
    };


    class StageDirector : public DeviceResourceBase
    {
      public:
        struct ReferenceBinding
        {
          public:
            const ResourceReferenceBase* Reference             = nullptr;
            int32_t                     ProvidedIndex         = -1;
            int32_t                     LastUsedIndex         = -1;
            uint64_t                     ImageRequirementsHash = 0UL;
            bool                         SurvivePresent        = false;
        };
        struct ImageResourceCount
        {
            StageImageInfo Info;
            uint32_t       Count;
        };
        using ImageCountSet = std::unordered_map<uint64_t, ImageResourceCount>;

        inline StageDirector(const VkContext* context = nullptr) : DeviceResourceBase("StageDirector"), mContext(context) {}

        virtual StageDirector& AddStage(RenderStage* stage);

        virtual void InitOrUpdate(const VkContext* context = nullptr);
        virtual void ReorderStages();
        virtual void RecreateAndSetImages(VkExtent2D swapchainSize);

        virtual void OnResized(Extent2D newsize);

        virtual bool Exists() const override;
        virtual void Cleanup() override;

        virtual ~StageDirector() { Cleanup(); }

        HSK_PROPERTY_ALL(Context)
        HSK_PROPERTY_ALLGET(Stages)
        HSK_PROPERTY_ALLGET(Images)

      protected:
        const VkContext*                                                           mContext = nullptr;
        std::vector<RenderStage*>                                                  mStages;
        std::vector<std::unique_ptr<FrameRotator<StageImage, INFLIGHT_FRAME_COUNT>>> mImages;
        std::vector<ReferenceBinding>                                              mReferenceBindings;
        ImageCountSet                                                              mGlobalCounts;
    };
}  // namespace hsk