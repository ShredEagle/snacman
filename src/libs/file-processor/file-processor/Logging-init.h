#pragma once

#include <memory>
#include <spdlog/logger.h>

namespace ad::processor {

std::shared_ptr<spdlog::logger> initializeLogger();

struct LoggerInitialization
{
    inline static const std::shared_ptr<spdlog::logger> logger =
        initializeLogger();
};

} // namespace ad::processor

extern "C"
{
    void ad_processor_loggerinitialization();
}
