#include "Logging-init.h"

#include "Logging.h"

#include <spdlog/sinks/stdout_color_sinks.h>


namespace ad::processor {


LoggerInitialization::LoggerInitialization()
{
    spdlog::stdout_color_mt(gMainLogger);
}


const LoggerInitialization LoggerInitialization::gInitialized;


} // namespace ad::processor


void ad_processor_loggerinitialization()
{
    // The hack to keep the symbol `LoggerInitialization::gInitialized`
    // works even though we are not accessing it.
}