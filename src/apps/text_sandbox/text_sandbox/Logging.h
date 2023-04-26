#pragma once


#include <spdlog/spdlog.h>


namespace ad {
namespace snac {


constexpr const char * gAppLogger = "text_sandbox";


// Add this namespace so initializeLogging() does not clash with the one from snacman.
namespace text_sandbox {

    /// \brief Safe initialization of the logger, guaranteed to happen only once.
    /// Must be called before any logging operation take place.
    void initializeLogging();

} // namespace text_sandbox


} // namespace snac
} // namespace ad


#define SELOG_LG(logger, severity) spdlog::get(logger)->severity
// Because we are in a leaf application, we can define the global logging in this header
#define SELOG(severity) SELOG_LG(gAppLogger, severity)