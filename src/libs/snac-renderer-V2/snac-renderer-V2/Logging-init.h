#pragma once

#include <memory>
#include <spdlog/logger.h>
#include <utility>

namespace ad::renderer {

std::pair<std::shared_ptr<spdlog::logger>, std::shared_ptr<spdlog::logger>>
initializeLogger();

struct LoggerInitialization
{
    inline static const std::pair<std::shared_ptr<spdlog::logger>,
                                  std::shared_ptr<spdlog::logger>>
        logger = initializeLogger();
};

} // namespace ad::renderer

extern "C"
{
    void ad_renderer_loggerinitialization();
}
