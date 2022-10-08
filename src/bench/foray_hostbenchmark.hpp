#pragma once
#include "foray_benchmarkbase.hpp"

namespace foray::bench {

    class HostBenchmark : public BenchmarkBase
    {
      public:
        void Begin();
        void LogTimestamp(const char* id);
        void End();
    };
}  // namespace foray