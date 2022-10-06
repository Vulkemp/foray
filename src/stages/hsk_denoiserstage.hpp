#pragma once
#include "hsk_renderstage.hpp"

namespace hsk {
    class DenoiserConfig
    {
      public:
        ManagedImage*              PrimaryInput = nullptr;
        ManagedImage*              AlbedoInput  = nullptr;
        ManagedImage*              NormalInput  = nullptr;
        std::vector<ManagedImage*> AuxiliaryInputs;
        ManagedImage*              PrimaryOutput = nullptr;
        void*                      AuxiliaryData = nullptr;
        VkSemaphore                Semaphore     = nullptr;

        inline DenoiserConfig() {}
        inline DenoiserConfig(ManagedImage* primaryIn, ManagedImage* albedoIn, ManagedImage* normalIn, ManagedImage* primaryOut, VkSemaphore semaphore)
            : PrimaryInput(primaryIn), AlbedoInput(albedoIn), NormalInput(normalIn), PrimaryOutput(primaryOut), Semaphore(semaphore)
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
