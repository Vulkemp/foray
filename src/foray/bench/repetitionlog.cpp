#include "repetitionlog.hpp"

namespace foray::bench
{
    void RepetitionLog::Append(std::string_view id, fp64_t delta)
    {
        Duration += delta;
        Intervals.emplace_back(Interval{id, delta});
    }
    void RepetitionLog::Append(util::StringSet* idSet, std::string_view id, fp64_t delta)
    {
        if(!!idSet)
        {
            id = idSet->Add(id);
        }
        Append(id, delta);
    }
} // namespace foray::bench
