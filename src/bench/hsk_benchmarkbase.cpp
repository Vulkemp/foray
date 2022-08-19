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
            << " | " << std::setw(12) << "Delta"
            << " | " << std::setw(12) << "Timestamp"
            << "\n" << std::right;

        {
            const auto& ts = Timestamps[0];
            out << "  " << std::setw(tsLen) << ts.Id << " | " << std::setw(12) << 0.0 << " | " << std::setw(12) << ts.Timestamp << "\n";
        }

        for(int32_t i = 1; i < Timestamps.size(); i++)
        {
            const auto& ts    = Timestamps[i];
            const auto& delta = Deltas[i - 1];
            out << "  " << std::setw(tsLen) << ts.Id << " | " << std::setw(12) << delta << " | " << std::setw(12) << ts.Timestamp << "\n";
        }

        {
            out << std::setw(tsLen + 2) << "Total" << " | " << std::setw(12) << End - Begin << " | " << std::setw(12) << "";
        }
        return out.str();
    }
    std::string BenchmarkLog::PrintCSVLines(char separator) const
    {
        std::stringstream out;
        out << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << std::fixed;
        for(const auto& ts : Timestamps)
        {
            out << ts.Id << separator << ts.Timestamp << "\n";
        }
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
        const auto& prev = mCurrentLog->Timestamps.back();
        mCurrentLog->Timestamps.emplace_back(BenchmarkTimestamp{.Id = id, .Timestamp = timestamp});
        mCurrentLog->Deltas.emplace_back(timestamp - prev.Timestamp);
    }
    void BenchmarkBase::End(fp64_t timestamp)
    {
        Assert(!!mCurrentLog, "Cannot end benchmark with no benchmark in progress! Call BenchmarkBase::Begin() first!");
        LogTimestamp(BenchmarkTimestamp::END, timestamp);
        mCurrentLog->End = timestamp;
        mCurrentLog      = nullptr;
    }
}  // namespace hsk