#include "foray_csvfilesink.hpp"

namespace foray::bench
{
    CsvFileLogSink::CsvFileLogSink(const osi::Utf8Path& path, char separator) : mHasPrintedHeader(false), mSeparator(separator)
    {
        mFile.open((std::filesystem::path)path, std::ios::out);
        Assert(mFile.is_open() && mFile.good());
    }
    CsvFileLogSink::~CsvFileLogSink()
    {
        mFile.flush();
        mFile.close();
    }
    void CsvFileLogSink::AppendLog(const RepetitionLog& log) 
    {
        if (!mHasPrintedHeader)
        {
            PrintHeaderInternal(log);
            mHasPrintedHeader = true;
        }
        PrintValuesInternal(log);
    }
    void CsvFileLogSink::PrintHeaderInternal(const RepetitionLog& log)
    {
        mFile << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << std::fixed;
        for(int32_t i = 0; i < (int32_t)log.Intervals.size() - 1; i++)
        {
            mFile << log.Intervals[i].Id << mSeparator;
        }
        mFile << log.Intervals.back().Id << "\n";
    }
    void CsvFileLogSink::PrintValuesInternal(const RepetitionLog& log)
    {
        mFile << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << std::fixed;
        for(int32_t i = 0; i < (int32_t)log.Intervals.size() - 1; i++)
        {
            mFile << log.Intervals[i].Delta << mSeparator;
        }
        mFile << log.Intervals.back().Delta << "\n";
    }
} // namespace foray::bench
