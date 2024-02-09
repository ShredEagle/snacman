#include "Logging-init.h"

#include "Logging.h"

#include <spdlog/sinks/stdout_color_sinks.h>


namespace ad::profiler {


LoggerInitialization::LoggerInitialization()
{
    spdlog::stdout_color_mt(gMainLogger);
}


const LoggerInitialization LoggerInitialization::gInitialized;


} // namespace ad::profiler


void ad_profiler_loggerinitialization()
{
    // The hack to keep the symbol `LoggerInitialization::gInitialized`
    // works even though we are not accessing it.
    //std::cout << ::ad::profiler::LoggerInitialization::gInitialized.i;
}