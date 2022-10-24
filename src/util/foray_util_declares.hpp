#pragma once

namespace foray::util {
    class DualBuffer;
    struct BufferSection;
    template <typename TClass>
    class ManagedVectorBuffer;
    class NoiseSource;
    class ExternalSemaphore;
    class ManagedUboBase;
    template <typename T_UBO>
    class ManagedUbo;
    class PipelineBuilder;
    class PipelineLayout;
    class ShaderStageCreateInfos;
}  // namespace foray::util
