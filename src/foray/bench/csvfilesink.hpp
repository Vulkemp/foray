#pragma once
#include "logsink.hpp"
#include "../osi/path.hpp"
#include <fstream>

namespace foray::bench
{
    class CsvFileLogSink : public ILogSink
    {
      public:
        CsvFileLogSink(const osi::Utf8Path& path, char separator = ';');
        virtual ~CsvFileLogSink();
        virtual void AppendLog(const RepetitionLog& log) override;

      protected:
        std::fstream mFile;
        bool         mHasPrintedHeader;
        char         mSeparator;

        void PrintHeaderInternal(const RepetitionLog& log);
        void PrintValuesInternal(const RepetitionLog& log);
    };
} // namespace foray::bench
