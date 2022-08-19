#include "hsk_benchmarkbase.hpp"
#include <iomanip>
#include <limits>
#include <sstream>
#include <string_view>

namespace hsk {

    std::string BenchmarkLog::PrintPretty() const
    {
        size_t tsLen = strlen("Id");
        for(const auto& ts : Timestamps)
        {
            tsLen = std::max(strlen(ts.Id), tsLen);
        }
        std::stringstream out;
        out << std::setprecision(5) << std::fixed;
        out << std::left << std::setw(tsLen + 2) << "Id"
            << " | " << std::setw(16) << "Delta"
            << " | " << std::setw(16) << "Timestamp"
            << "\n"
            << std::right;

        {
            const auto& ts = Timestamps[0];
            out << "  " << std::setw(tsLen) << ts.Id << " | " << std::setw(16) << "" << " | " << std::setw(16) << ts.Timestamp << "\n";
        }

        for(int32_t i = 1; i < Timestamps.size(); i++)
        {
            const auto& ts    = Timestamps[i];
            const auto& delta = Deltas[i - 1];
            out << "  " << std::setw(tsLen) << ts.Id << " | " << std::setw(16) << delta << " | " << std::setw(16) << ts.Timestamp << "\n";
        }

        {
            out << std::setw(tsLen + 2) << "Total"
                << " | " << std::setw(16) << End - Begin << " | " << std::setw(16) << "";
        }
        return out.str();
    }
    std::string BenchmarkLog::PrintCSVLine(char separator) const
    {
        std::stringstream out;
        out << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << std::fixed;
        for(int32_t i = 0; i < Timestamps.size() - 1; i++)
        {
            out << Timestamps[i].Timestamp << separator;
        }
        out << Timestamps.back().Timestamp;
        return out.str();
    }

    BenchmarkBase::BenchmarkBase() {}

    void BenchmarkBase::Begin(fp64_t timestamp)
    {
        Assert(!mCurrentLog, "Can not begin new benchmark when a benchmark is already running! End previous benchmark with BenchmarkBase::End() first!");
        mCurrentLog        = &(mLogs.emplace_back(BenchmarkLog{}));
        mCurrentLog->Begin = timestamp;
        mCurrentLog->Timestamps.emplace_back(BenchmarkTimestamp{.Id = BenchmarkTimestamp::BEGIN, .Timestamp = timestamp});
    }
    void BenchmarkBase::LogTimestamp(const char* id, fp64_t timestamp)
    {
        Assert(!!mCurrentLog, "Cannot log timestamp with no benchmark in progress! Call BenchmarkBase::Begin() first!");
        fp64_t prev = mCurrentLog->Timestamps.back().Timestamp;
        mCurrentLog->Timestamps.emplace_back(BenchmarkTimestamp{.Id = id, .Timestamp = timestamp});
        mCurrentLog->Deltas.emplace_back(timestamp - prev);
    }
    void BenchmarkBase::End(fp64_t timestamp)
    {
        Assert(!!mCurrentLog, "Cannot end benchmark with no benchmark in progress! Call BenchmarkBase::Begin() first!");
        LogTimestamp(BenchmarkTimestamp::END, timestamp);
        mCurrentLog->End = timestamp;
        mCurrentLog      = nullptr;
    }
}  // namespace hsk