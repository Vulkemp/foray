#include "foray_hostbenchmark.hpp"
#include <chrono>

namespace foray::bench {
    using namespace std::chrono;

    inline static high_resolution_clock             sClock    = high_resolution_clock();
    inline static high_resolution_clock::time_point sRefpoint = sClock.now();

    fp64_t lGetTimestamp()
    {
        return duration<fp64_t, std::milli>(sClock.now() - sRefpoint).count();
    }

    HostBenchmark::HostBenchmark(bool useIdSet) : mLast(0.0), mPause(0.0), mIsPaused(false), mIdSet()
    {
        if(useIdSet)
        {
            mIdSet.New();
        }
    }

    HostBenchmark::~HostBenchmark()
    {
        Finalize();
    }

    void HostBenchmark::LogTimestamp(std::string_view id)
    {
        Assert(mRecording, "Must start first!");
        if(mIsPaused)
        {
            Resume();
        }
        fp64_t now = lGetTimestamp();
        fp64_t delta = now - mLast;
        mRecording->Append(mIdSet.GetNullable(), id, delta);
        mLast = now;
    }
    void HostBenchmark::Start()
    {
        if(!mRecording)
        {
            mRecording.New();
        }
        mLast     = lGetTimestamp();
        mIsPaused = false;
    }
    void HostBenchmark::Finalize(std::string_view id)
    {
        if(!id.empty())
        {
            LogTimestamp(id);
        }
        if(mRecording)
        {
            mOnLogFinalized.Invoke(mRecording.GetRef());
            mRecording.Delete();
        }
    }
    void HostBenchmark::Pause()
    {
        if(!mIsPaused)
        {
            mPause    = lGetTimestamp();
            mIsPaused = true;
        }
    }
    void HostBenchmark::Resume()
    {
        if(mIsPaused)
        {
            fp64_t pausedDelta = lGetTimestamp() - mPause;
            mLast += pausedDelta;
            mIsPaused = false;
        }
    }
}  // namespace foray::bench