#include "fmtutilities.hpp"
#include <spdlog/fmt/fmt.h>

namespace foray::util {
    std::string PrintSize(size_t size)
    {
        const size_t KiB = 1024;
        const size_t MiB = KiB * 1024;
        const size_t GiB = MiB * 1024;
        const size_t TiB = GiB * 1024;
        if(size < KiB)
        {
            return fmt::format("{} B", size);
        }
        if(size < MiB)
        {
            double val = (double)size / (double)KiB;
            return fmt::format("{:.3f} KiB", val);
        }
        if(size < GiB)
        {
            double val = (double)size / (double)MiB;
            return fmt::format("{:.3f} MiB", val);
        }
        if(size < TiB)
        {
            double val = (double)size / (double)GiB;
            return fmt::format("{:.3f} GiB", val);
        }
        else
        {
            double val = (double)size / (double)TiB;
            return fmt::format("{:.3f} PiB", val);
        }
    }
}  // namespace foray::util