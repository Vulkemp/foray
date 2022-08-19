#pragma once
#include "../hsk_basics.hpp"

namespace hsk {
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

        std::string PrintPretty() const;
        std::string PrintCSVLines(char separator = ';') const;
        void PrintImGui();
    };

    class BenchmarkBase
    {
      public:
        BenchmarkBase();

        HSK_PROPERTY_CGET(Logs)

      protected:
        virtual void Begin(fp64_t timestamp);
        virtual void LogTimestamp(const char* id, fp64_t timestamp);
        virtual void End(fp64_t timestamp);

        std::vector<BenchmarkLog> mLogs;
        BenchmarkLog*             mCurrentLog = nullptr;
    };
}  // namespace hsk