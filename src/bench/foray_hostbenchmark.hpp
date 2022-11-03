#pragma once
#include "foray_benchmarkbase.hpp"

namespace foray::bench {

    /// @brief A host (CPU time) benchmark based on std::chrono::high_resolution_clock
    class HostBenchmark : public BenchmarkBase
    {
      public:
        /// @brief Begins a new benchmark and records the "Begin" timestamp
        void Begin();
        /// @brief Logs timestamp of id
        void LogTimestamp(const char* id);
        /// @brief Records the "End" timestamp and finalizes the benchmark
        void End();
    };
}  // namespace foray