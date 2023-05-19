#pragma once
#include "benchmarkbase.hpp"

namespace foray::bench {

    /// @brief A host (CPU time) benchmark based on std::chrono::high_resolution_clock. Timestamps are recorded in milliseconds.
    class HostBenchmark : public BenchmarkBase
    {
      public:
        explicit HostBenchmark(bool useIdSet);
        virtual ~HostBenchmark();

        /// @brief Logs timestamp of id
        void LogTimestamp(std::string_view id);
        void Start();
        void Finalize(std::string_view id = "");
        void Pause();
        void Resume();

      protected:
        fp64_t                 mLast;
        fp64_t                 mPause;
        bool                   mIsPaused;
        Local<util::StringSet> mIdSet;
        Local<RepetitionLog>   mRecording;
    };
}  // namespace foray::bench