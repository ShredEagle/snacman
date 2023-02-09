#pragma once


#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h> // allow use of operator<<



namespace ad {
namespace snac {


constexpr const char * gRenderLogger = "snac-render";
constexpr const char * gGltfLogger = "gltf-loader";


// Add this namespace so initializeLogging() does not clash with the one from snacman.
namespace renderer {

    /// \brief Safe initialization of the logger, guaranteed to happen only once.
    /// Must be called before any logging operation take place.
    void initializeLogging();

} // namespace renderer


} // namespace snac
} // namespace ad


#define SELOG_LG(logger, severity) spdlog::get(logger)->severity