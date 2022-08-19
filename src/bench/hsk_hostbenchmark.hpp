#pragma once
#include "hsk_benchmarkbase.hpp"

namespace hsk {

    class HostBenchmark : public BenchmarkBase
    {
      public:
        void Begin();
        void LogTimestamp(const char* id);
        void End();
    };
}  // namespace hsk