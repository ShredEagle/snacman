#include "Logging-init.h"

#include "Logging.h"

#include <spdlog/sinks/stdout_color_sinks.h>


namespace ad::renderer {


LoggerInitialization::LoggerInitialization()
{
    spdlog::stdout_color_mt(gMainLogger);
}


const LoggerInitialization LoggerInitialization::gInitialized;


} // namespace ad::renderer


void ad_renderer_loggerinitialization()
{
    // The hack to keep the symbol `LoggerInitialization::gInitialized`
    // works even though we are not accessing it.
}