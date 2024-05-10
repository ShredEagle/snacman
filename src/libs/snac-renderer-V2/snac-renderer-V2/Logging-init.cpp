#include "Logging-init.h"

#include "Logging.h"

#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <utility>

namespace ad::renderer {

std::pair<std::shared_ptr<spdlog::logger>, std::shared_ptr<spdlog::logger>>
initializeLogger()
{
    return std::make_pair(spdlog::stdout_color_mt(gMainLogger),
                          spdlog::stdout_color_mt(gPipelineDiag));
}
} // namespace ad::renderer

// void ad_renderer_loggerinitialization()
// {
//     // The hack to keep the symbol `LoggerInitialization::gInitialized`
//     // works even though we are not accessing it.
//     std::cout << ad::renderer::LoggerInitialization::gInitialized.i;
// }
