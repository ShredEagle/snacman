#include "Logging-init.h"

#include "Logging.h"

#include <iostream>
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>


namespace ad::processor {


std::shared_ptr<spdlog::logger> initializeLogger()
{
    return spdlog::stdout_color_mt(gMainLogger);
}

} // namespace ad::processor


// void ad_processor_loggerinitialization()
// {
//     // The hack to keep the symbol `LoggerInitialization::gInitialized`
//     // works even though we are not accessing it.
//     std::cout << ad::processor::LoggerInitialization::gInitialized.i;
// }
