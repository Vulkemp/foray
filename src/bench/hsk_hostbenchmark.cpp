#include "hsk_hostbenchmark.hpp"
#include <chrono>

namespace hsk {
    using namespace std::chrono;

    inline static high_resolution_clock             sClock    = high_resolution_clock();
    inline static high_resolution_clock::time_point sRefpoint = sClock.now();

    fp64_t lGetTimestamp()
    {
        return duration<fp64_t, std::milli>(sClock.now() - sRefpoint).count();
    }

    void HostBenchmark::Begin()
    {
        BenchmarkBase::Begin(lGetTimestamp());
    }

    void HostBenchmark::LogTimestamp(const char* id)
    {
        BenchmarkBase::LogTimestamp(id, lGetTimestamp());
    }
    void HostBenchmark::End()
    {
        BenchmarkBase::End(lGetTimestamp());
    }
}  // namespace hsk