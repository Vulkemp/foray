#include "foray_benchmarkbase.hpp"
#include "../foray_exception.hpp"
#include <imgui/imgui.h>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string_view>

namespace foray::bench {

    std::string BenchmarkLog::PrintPretty(bool omitTimestamps) const
    {
        size_t tsLen = strlen("Id");
        for(const auto& ts : Timestamps)
        {
            tsLen = std::max(strlen(ts.Id), tsLen);
        }
        std::stringstream out;
        out << std::setprecision(5) << std::fixed;
        out << std::left << std::setw(tsLen + 2) << "Id"
            << " | " << std::setw(16) << "Delta";
        if(!omitTimestamps)
        {
            out << " | " << std::setw(16) << "Timestamp";
        }
        out << "\n" << std::right;

        {
            const auto& ts = Timestamps[0];
            out << "  " << std::setw(tsLen) << ts.Id << " | " << std::setw(16) << "";
            if(!omitTimestamps)
            {
                out << " | " << std::setw(16) << ts.Timestamp;
            }
            out << "\n";
        }

        for(int32_t i = 1; i < Timestamps.size(); i++)
        {
            const auto& ts    = Timestamps[i];
            const auto& delta = Deltas[i - 1];
            out << "  " << std::setw(tsLen) << ts.Id << " | " << std::setw(16) << delta << " ms";
            if(!omitTimestamps)
            {
                out << " | " << std::setw(16) << ts.Timestamp;
            }
            out << "\n";
        }

        {
            out << std::setw(tsLen + 2) << "Total"
                << " | " << std::setw(16) << End - Begin << " ms";
            if(!omitTimestamps)
            {
                out << " | " << std::setw(16) << "";
            }
        }
        return out.str();
    }
    std::string BenchmarkLog::PrintCsvLine(char separator, bool includeNewLine) const
    {
        std::stringstream out;
        out << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << std::fixed;
        for(int32_t i = 0; i < Timestamps.size() - 1; i++)
        {
            out << Timestamps[i].Timestamp << separator;
        }
        out << Timestamps.back().Timestamp;
        if (includeNewLine)
        {
            out << "\n";
        }
        return out.str();
    }
    std::string BenchmarkLog::PrintCsvHeader(char separator, bool includeNewLine) const
    {
        std::stringstream out;
        out << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << std::fixed;
        for(int32_t i = 0; i < Timestamps.size() - 1; i++)
        {
            out << Timestamps[i].Id << separator;
        }
        out << Timestamps.back().Id;
        if (includeNewLine)
        {
            out << "\n";
        }
        return out.str();
    }

    void BenchmarkLog::PrintImGui(bool omitTimestamps)
    {
        if(ImGui::BeginTable("Benchmark", omitTimestamps ? 2 : 3))
        {
            ImGui::TableSetupColumn("Id");
            ImGui::TableSetupColumn("Delta");
            if(!omitTimestamps)
            {
                ImGui::TableSetupColumn("Timestamp");
            }
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableHeader("Id");
            ImGui::TableNextColumn();
            ImGui::TableHeader("Delta");
            if(!omitTimestamps)
            {
                ImGui::TableNextColumn();
                ImGui::TableHeader("Timestamp");
            }

            {
                const auto& ts = Timestamps[0];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(ts.Id);
                ImGui::TableNextColumn();
                if(!omitTimestamps)
                {
                    ImGui::TableNextColumn();
                    ImGui::Text("%f", ts.Timestamp);
                }
            }

            for(int32_t i = 1; i < Timestamps.size(); i++)
            {
                const auto& ts    = Timestamps[i];
                const auto& delta = Deltas[i - 1];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(ts.Id);
                ImGui::TableNextColumn();
                ImGui::Text("%f ms", delta);
                if(!omitTimestamps)
                {
                    ImGui::TableNextColumn();
                    ImGui::Text("%f", ts.Timestamp);
                }
            }

            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Total");
                ImGui::TableNextColumn();
                ImGui::Text("%f ms", End - Begin);
            }

            ImGui::EndTable();
        }
    }

    BenchmarkBase::BenchmarkBase() {}

    void BenchmarkBase::Begin(fp64_t timestamp)
    {
        Assert(!mRecording, "Can not begin new benchmark when a benchmark is already running! End previous benchmark with BenchmarkBase::End() first!");
        mRecording = true;
        mCurrentLog        = BenchmarkLog{};
        mCurrentLog.Begin = timestamp;
        mCurrentLog.Timestamps.emplace_back(BenchmarkTimestamp{.Id = BenchmarkTimestamp::BEGIN, .Timestamp = timestamp});
    }
    void BenchmarkBase::LogTimestamp(const char* id, fp64_t timestamp)
    {
        Assert(mRecording, "Cannot log timestamp with no benchmark in progress! Call BenchmarkBase::Begin() first!");
        fp64_t prev = mCurrentLog.Timestamps.back().Timestamp;
        mCurrentLog.Timestamps.emplace_back(BenchmarkTimestamp{.Id = id, .Timestamp = timestamp});
        mCurrentLog.Deltas.emplace_back(timestamp - prev);
    }
    void BenchmarkBase::End(fp64_t timestamp)
    {
        Assert(mRecording, "Cannot end benchmark with no benchmark in progress! Call BenchmarkBase::Begin() first!");
        LogTimestamp(BenchmarkTimestamp::END, timestamp);
        mCurrentLog.End = timestamp;
        mRecording = false;
        mLogs.emplace_back(mCurrentLog);
    }
}  // namespace foray::bench