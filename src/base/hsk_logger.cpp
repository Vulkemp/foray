#include "hsk_logger.hpp"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace hsk {
    spdlog::logger* logger()
    {
        static spdlog::logger* defaultLogger{nullptr};

        if(defaultLogger != nullptr)
            return defaultLogger;

        // else create default logger
        auto consoleSink       = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto fileSizeMegabytes = 5;
        auto maxSize           = 1048576 * fileSizeMegabytes;
        auto maxFiles          = 5;
        auto fileSink          = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/log.txt", maxSize, maxFiles, true);

        // create a logger that logs both to console and to logfiles in a rotating manner
        defaultLogger = new spdlog::logger("main_logger", {consoleSink, fileSink});
        return defaultLogger;
    }
}  // namespace hsk