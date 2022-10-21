#pragma once
#include "foray_renderstage.hpp"
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace foray::stages {
    class DenoiserSynchronisationSemaphore : public core::DeviceResourceBase
    {
      public:
        void                Create(core::Context* context);
        inline virtual bool Exists() const override { return !!mSemaphore; }
        virtual void        Destroy() override;

        FORAY_PROPERTY_CGET(Semaphore)
        FORAY_PROPERTY_CGET(Handle)
        FORAY_PROPERTY_ALL(Value)

      protected:
        core::Context* mContext   = nullptr;
        VkSemaphore            mSemaphore = nullptr;
        uint64_t               mValue     = 0;
#ifdef WIN32
        HANDLE mHandle = INVALID_HANDLE_VALUE;
#else
        int mHandle = -1;
#endif
    };

    class DenoiserConfig
    {
      public:
        core::ManagedImage*               PrimaryInput = nullptr;
        core::ManagedImage*               AlbedoInput  = nullptr;
        core::ManagedImage*               NormalInput  = nullptr;
        std::vector<core::ManagedImage*>  AuxiliaryInputs;
        core::ManagedImage*               PrimaryOutput = nullptr;
        void*                             AuxiliaryData = nullptr;
        DenoiserSynchronisationSemaphore* Semaphore     = nullptr;

        inline DenoiserConfig() {}
        inline DenoiserConfig(
            core::ManagedImage* primaryIn, core::ManagedImage* albedoIn, core::ManagedImage* normalIn, core::ManagedImage* primaryOut, DenoiserSynchronisationSemaphore& semaphore)
            : PrimaryInput(primaryIn), AlbedoInput(albedoIn), NormalInput(normalIn), PrimaryOutput(primaryOut), Semaphore(&semaphore)
        {
        }
    };

    class DenoiserStage : public RenderStage
    {
      public:
        virtual void Init(core::Context* context, const DenoiserConfig& config){};

        virtual void BeforeDenoise(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo){};
        virtual void AfterDenoise(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo){};
        virtual void DispatchDenoise(uint64_t& timelineValue){};
    };
}  // namespace foray::stages
