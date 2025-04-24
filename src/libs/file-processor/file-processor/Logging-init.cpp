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