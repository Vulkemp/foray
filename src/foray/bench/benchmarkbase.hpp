#pragma once
#include "../basics.hpp"
#include "../event.hpp"
#include "../logger.hpp"
#include "../osi/path.hpp"
#include "repetitionlog.hpp"
#include <fstream>

namespace foray::bench {

    class BenchmarkBase
    {
        FORAY_DELEGATE(const RepetitionLog&, LogFinalized)
    };

    template <class TIBenchmarkSink>
    class BenchmarkForward
    {
      public:
        template <typename... TArgs>
        BenchmarkForward(BenchmarkBase* benchmark, TArgs&&... args)
            : Sink(args...), Receiver(benchmark->OnLogFinalized(), [this](const RepetitionLog& log) { this->Sink.AppendLog(log); })
        {
        }

        TIBenchmarkSink                       Sink;
        event::Receiver<const RepetitionLog&> Receiver;
    };
}  // namespace foray::bench