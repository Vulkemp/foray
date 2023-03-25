#pragma once
#include "../bench/foray_bench_declares.hpp"
#include "../util/foray_externalsemaphore.hpp"
#include "foray_renderstage.hpp"

namespace foray::stages {

    /// @brief Config struct for initialization of denoiser stages
    struct DenoiserConfig
    {
        RenderDomain* Domain = nullptr;
        /// @brief The primary input. Noisy linear radiance data, usually from the raytracer directly.
        core::ManagedImage* PrimaryInput = nullptr;
        /// @brief Misc. input Images (e.g. Mesh Id, Material Id, History data)
        std::unordered_map<std::string, core::ManagedImage*> AuxiliaryInputs;
        /// @brief The primary output. Denoised Image from the denoiser
        core::ManagedImage* PrimaryOutput = nullptr;
        /// @brief Customizable pointer to auxiliary data
        void* AuxiliaryData = nullptr;
        /// @brief Semaphore for synchronisation of externally computing denoisers (e.g. OptiX Denoiser)
        util::ExternalSemaphore* Semaphore = nullptr;
        bench::DeviceBenchmark*  Benchmark = nullptr;

        inline DenoiserConfig() {}
        inline DenoiserConfig(core::ManagedImage* primaryIn, core::ManagedImage* primaryOut) : PrimaryInput(primaryIn), PrimaryOutput(primaryOut) {}
    };

    /// @brief Base class for denoiser implementations
    class DenoiserStage : public RenderStage
    {
      public:
        virtual void Init(core::Context* context, const DenoiserConfig& config, int32_t resizeOrder = 0) { RenderStage::InitCallbacks(context, config.Domain, resizeOrder); };
        virtual void UpdateConfig(const DenoiserConfig& config){};
        /// @brief Get the UI label used for user facing labelling of the denoiser
        virtual std::string GetUILabel() { return "Unnamed Denoiser"; }
        /// @brief Record the ImGui commands for configuration of the denoiser
        virtual void DisplayImguiConfiguration(){};
        /// @brief Signals the denoiser stage that history information is to be ignored for the coming frame
        virtual void IgnoreHistoryNextFrame(){};
    };

    /// @brief Base class for externally computing denoiser implementations (e.g. OptiX Denoiser)
    /// @remark The Semaphore member of DenoiserConfig is expected to be used for synchronization
    class ExternalDenoiserStage : public DenoiserStage
    {
      public:
        /// @brief Vulkan side work to be computed before doing external work
        virtual void BeforeDenoise(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo){};
        /// @brief Vulkan side work to be computed after doing external work
        virtual void AfterDenoise(VkCommandBuffer cmdBuffer, base::FrameRenderInfo& renderInfo){};
        /// @brief Function for dispatching Api-external work
        /// @param timelineSemaphoreValueBefore Timeline Semaphore Value the external api should work on before running
        /// @param timelineSemaphoreValueAfter Timeline Semaphore Value the external api should signal after completing
        virtual void DispatchDenoise(uint64_t timelineSemaphoreValueBefore, uint64_t timelineSemaphoreValueAfter){};
    };
}  // namespace foray::stages
