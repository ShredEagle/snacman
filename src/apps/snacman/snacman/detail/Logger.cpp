#include "Logger.h"

#include <arte/Logging.h>
#include <snac-renderer-V1/Logging.h>
#include <sounds/Logging.h>

#include <spdlog/sinks/stdout_color_sinks.h>

#include <mutex>


namespace ad {
namespace snac {
namespace detail {


namespace {

    std::once_flag loggingInitializationOnce;

    void initializeLogging_impl()
    {
        // Intended for the main game messages
        spdlog::stdout_color_mt(gMainLogger);
    }

} // namespace anonymous


void initializeLogging()
{
    std::call_once(loggingInitializationOnce, &initializeLogging_impl);
    arte::initializeLogging();
    renderer::initializeLogging();
    sounds::initializeLogging();

    //Note: Initialize all upstream libraries logging here too.
};


} // namespace detail
} // namespace snac
} // namespace ad
