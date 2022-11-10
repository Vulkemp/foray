#pragma once
#include "../foray_basics.hpp"
#include <vector>

namespace foray::bench {
    /// @brief Timestamp combining a label / id with a timestamp in milliseconds
    class BenchmarkTimestamp
    {
      public:
        static inline const char* BEGIN = "Begin";
        static inline const char* END   = "End";

        /// @brief Id of the data point for identification
        const char* Id = "no Id";
        /// @brief Timestamp in milliseconds
        fp64_t Timestamp = 0.0;
    };

    /// @brief Log of a single benchmark run. All timestamps given must be relative to the same base time
    class BenchmarkLog
    {
      public:
        /// @brief Value of the first recorded timestamp
        fp64_t Begin = 0.0;
        /// @brief Value of the last recorded timestamp
        fp64_t End = 0.0;
        /// @brief List of timestamps in chronological order. Guaranteed to begin with BenchmarkTimestamp::Begin and end with BenchmarkTimestamp::End
        std::vector<BenchmarkTimestamp> Timestamps;
        /// @brief Deltas between the timestamps in milliseconds
        std::vector<fp64_t> Deltas;

        /// @brief Prints a table with all timestamps and deltas, aswell as a total delta
        std::string PrintPretty(bool omitTimestamps = true) const;
        /// @brief Prints all timestamps, separated by the separator character
        std::string PrintCSVLine(char separator = ';') const;
        /// @brief Execute imgui command sequence to display the benchmark results
        void PrintImGui(bool omitTimestamps = true);
    };

    /// @brief Base class for all benchmark types
    class BenchmarkBase
    {
      public:
        BenchmarkBase();

        FORAY_GETTER_MR(Logs)
        FORAY_GETTER_CR(Logs)

      protected:
        /// @brief Begins a new benchmark and records the "Begin" timestamp
        virtual void Begin(fp64_t timestamp);
        /// @brief Logs timestamp of id
        virtual void LogTimestamp(const char* id, fp64_t timestamp);
        /// @brief Records the "End" timestamp and finalizes the benchmark
        virtual void End(fp64_t timestamp);

        std::vector<BenchmarkLog> mLogs;
        BenchmarkLog              mCurrentLog;
        bool                      mRecording = false;
    };
}  // namespace foray::bench