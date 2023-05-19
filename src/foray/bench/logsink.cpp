#include "logsink.hpp"
#include <imgui/imgui.h>
#include <iomanip>
#include <iostream>

namespace foray::bench {
    LoggerSink::LoggerSink(std::string_view name, spdlog::level::level_enum level) : mLogger(logger()), mLevel(level) {}
    LoggerSink::LoggerSink(std::string_view name, spdlog::logger* logger, spdlog::level::level_enum level) : mLogger(logger), mLevel(level) {}
    void LoggerSink::AppendLog(const RepetitionLog& log)
    {
        size_t tsLen = strlen("Id") + 2;
        for(const auto& ts : log.Intervals)
        {
            tsLen = std::max(ts.Id.size() + 2, tsLen);
        }
        std::stringstream out;
        out << std::setprecision(5) << std::fixed;

        // Name
        out << mName << "\n";

        // Header
        out << std::left << std::setw(tsLen) << "Id"
            << " | " << std::setw(16) << "Delta"
            << "\n"
            << std::right;

        // Separator
        out << std::string(tsLen + 22, '=') << "\n";

        // Values
        for(const Interval& interval : log.Intervals)
        {
            out << std::setw(tsLen) << interval.Id << " | " << std::setw(16) << interval.Delta << " ms\n";
        }

        // Separator
        out << std::string(tsLen + 22, '-') << "\n";

        // Total
        {
            out << std::setw(tsLen) << "Total"
                << " | " << std::setw(16) << log.Duration << " ms";
        }

        mLogger->log(mLevel, out.str());
    }

    void ImguiLogSink::DisplayImguiTableInternal(bool displayName)
    {
        std::string_view name = "";
        if(displayName)
        {
            if(mName.empty())
            {
                name = "Benchmark";
            }
            else
            {
                name = mName;
            }
        }
        if(ImGui::BeginTable(name.data(), 2))
        {
            ImGui::TableSetupColumn("Id");
            ImGui::TableSetupColumn("Delta");
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableHeader("Id");
            ImGui::TableNextColumn();
            ImGui::TableHeader("Delta");

            for(int32_t i = 0; i < (int32_t)mLog.Intervals.size(); i++)
            {
                const auto& ts = mLog.Intervals[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(ts.Id.data(), ts.Id.end());
                ImGui::TableNextColumn();
                ImGui::Text("%f ms", ts.Delta);
            }

            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Total");
                ImGui::TableNextColumn();
                ImGui::Text("%f ms", mLog.Duration);
            }

            ImGui::EndTable();
        }
    }
    void ImguiLogSink::DisplayImguiTable()
    {
        DisplayImguiTableInternal(true);
    }
    void ImguiLogSink::DisplayImguiWindow()
    {
        std::string_view nameFallback = "Benchmark";
        std::string_view name         = mName.empty() ? nameFallback : std::string_view(mName);
        if(ImGui::Begin(name.data()))
        {
            DisplayImguiTableInternal(false);
        }
        ImGui::End();
    }
    void ImguiLogSink::AppendLog(const RepetitionLog& log)
    {
        mLog = log;
    }
}  // namespace foray::bench