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
            uint32_t                     ProvidedIndex         = -1;
            uint32_t                     LastUsedIndex         = -1;
            uint64_t                     ImageRequirementsHash = 0UL;
        };

        inline StageDirector(const VkContext* context = nullptr) : DeviceResourceBase("StageDirector"), mContext(context) {}

        virtual StageDirector& AddStage(RenderStage* stage);

        virtual void InitOrUpdate(const VkContext* context = nullptr);

        virtual bool Exists() const override;
        virtual void Cleanup() override;

        virtual ~StageDirector() { Cleanup(); }

        HSK_PROPERTY_ALL(Context)
        HSK_PROPERTY_ALLGET(Stages)
        HSK_PROPERTY_ALLGET(Images)

      protected:
        const VkContext*                                                                   mContext = nullptr;
        std::vector<RenderStage*>                                                          mStages;
        std::unordered_map<std::string_view, FrameRotator<StageImage, INFLIGHTFRAMECOUNT>> mImages;
        std::vector<ReferenceBinding>                                                      mReferenceBindings;
    };
}  // namespace hsk