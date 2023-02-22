#include "Logging.h"

#include <arte/Logging.h>

#include <spdlog/sinks/stdout_color_sinks.h>

#include <mutex>


namespace ad {
namespace snac {


namespace {

    std::once_flag loggingInitializationOnce;

    void initializeLogging_impl()
    {
        // Intended for the main game messages
        spdlog::stdout_color_mt(gRenderLogger);
        spdlog::stdout_color_mt(gGltfLogger);
        spdlog::stdout_color_mt(gResourceLogger);
    }

} // namespace anonymous


namespace renderer {

    void initializeLogging()
    {
        std::call_once(loggingInitializationOnce, &initializeLogging_impl);
        arte::initializeLogging();
        //Note: Initialize all upstream libraries logging here too.
    };

} // namespace renderer


} // namespace snac
} // namespace ad
