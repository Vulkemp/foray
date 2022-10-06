#pragma once
#include "hsk_renderstage.hpp"

namespace hsk {
    class DenoiserSynchronisationSemaphore : public DeviceResourceBase
    {
      public:
        void                Create(const VkContext* context);
        inline virtual bool Exists() const override { return !!mSemaphore; }
        virtual void        Destroy() override;

        HSK_PROPERTY_CGET(Semaphore)
        HSK_PROPERTY_CGET(Handle)
        HSK_PROPERTY_ALL(Value)

      protected:
        const VkContext* mContext   = nullptr;
        VkSemaphore      mSemaphore = nullptr;
        uint64_t         mValue     = 0;
#ifdef WIN32
        HANDLE mHandle = INVALID_HANDLE_VALUE;
#else
        int mHandle = -1;
#endif
    };

    class DenoiserConfig
    {
      public:
        ManagedImage*                     PrimaryInput = nullptr;
        ManagedImage*                     AlbedoInput  = nullptr;
        ManagedImage*                     NormalInput  = nullptr;
        std::vector<ManagedImage*>        AuxiliaryInputs;
        ManagedImage*                     PrimaryOutput = nullptr;
        void*                             AuxiliaryData = nullptr;
        DenoiserSynchronisationSemaphore* Semaphore     = nullptr;

        inline DenoiserConfig() {}
        inline DenoiserConfig(ManagedImage* primaryIn, ManagedImage* albedoIn, ManagedImage* normalIn, ManagedImage* primaryOut, DenoiserSynchronisationSemaphore& semaphore)
            : PrimaryInput(primaryIn), AlbedoInput(albedoIn), NormalInput(normalIn), PrimaryOutput(primaryOut), Semaphore(&semaphore)
        {
        }
    };

    class DenoiserStage : public RenderStage
    {
      public:
        virtual void Init(const VkContext* context, const DenoiserConfig& config){};

        virtual void BeforeDenoise(const FrameRenderInfo& renderInfo){};
        virtual void AfterDenoise(const FrameRenderInfo& renderInfo){};
        virtual void DispatchDenoise(uint64_t& timelineValue){};
    };
}  // namespace hsk
