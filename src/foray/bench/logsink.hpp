#pragma once
#include "../basics.hpp"
#include "../logger.hpp"
#include "repetitionlog.hpp"

namespace foray::bench
{
    class ILogSink
    {
      public:
        virtual ~ILogSink()                       = default;
        virtual void AppendLog(const RepetitionLog& log) = 0;
    };

    class LoggerSink : public ILogSink
    {
      public:
        LoggerSink(std::string_view name, spdlog::level::level_enum level = spdlog::level::info);
        LoggerSink(std::string_view name, spdlog::logger* logger, spdlog::level::level_enum level = spdlog::level::info);
        virtual void AppendLog(const RepetitionLog& log) override;

      protected:
        std::string mName;
        spdlog::logger*           mLogger;
        spdlog::level::level_enum mLevel;
    };

    class ImguiLogSink : public ILogSink
    {
      public:
        inline explicit ImguiLogSink(std::string_view name = "") : mName(name) {}

        void         DisplayImguiTable();
        void         DisplayImguiWindow();
        virtual void AppendLog(const RepetitionLog& log) override;

      protected:
        void         DisplayImguiTableInternal(bool displayName);
        std::string  mName;
        RepetitionLog mLog;
    };
} // namespace foray::bench
