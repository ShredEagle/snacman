#pragma once

#include <memory>
#include <spdlog/logger.h>

namespace ad::profiler {

std::shared_ptr<spdlog::logger> initializeLogger();

struct LoggerInitialization
{
    inline static const std::shared_ptr<spdlog::logger> logger =
        initializeLogger();
};

} // namespace ad::profiler

extern "C"
{
    void ad_profiler_loggerinitialization();
}
