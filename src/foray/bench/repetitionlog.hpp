#pragma once
#include "../basics.hpp"
#include "../util/stringset.hpp"

namespace foray::bench
{
    /// @brief Timestamp combining a label / id with a timestamp in milliseconds
    struct Interval
    {
        /// @brief Id of the data point for identification
        std::string_view Id = "no Id";
        /// @brief Timestamp in milliseconds
        fp64_t Delta = 0.0;
    };

    /// @brief Log of a single benchmark run. All timestamps given must be relative to the same base time
    struct RepetitionLog
    {
        /// @brief Value of the last recorded timestamp
        fp64_t Duration = 0.0;
        /// @brief List of timestamps in chronological order. Guaranteed to begin with Timestamp::Begin and end with Timestamp::End
        std::vector<Interval> Intervals;

        void Append(std::string_view id, fp64_t delta);
        void Append(util::StringSet* idSet, std::string_view id, fp64_t delta);
    };

} // namespace foray::bench
