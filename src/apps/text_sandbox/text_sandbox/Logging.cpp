#include "Logging.h"

#include <snac-renderer-V1/Logging.h>

#include <spdlog/sinks/stdout_color_sinks.h>

#include <mutex>


namespace ad {
namespace snac {


namespace {

    std::once_flag loggingInitializationOnce;

    void initializeLogging_impl()
    {
        // Intended for the main game messages
        spdlog::stdout_color_mt(gAppLogger);
    }

} // namespace anonymous


namespace text_sandbox {

    void initializeLogging()
    {
        std::call_once(loggingInitializationOnce, &initializeLogging_impl);
        snac::renderer::initializeLogging();
        //Note: Initialize all upstream libraries logging here too.
    };

} // namespace text_sandbox


} // namespace snac
} // namespace ad
