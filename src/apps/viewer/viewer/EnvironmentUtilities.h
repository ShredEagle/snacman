#pragma once

#include <filesystem>

#include <renderer/Shading.h>


namespace ad::renderer {


struct Environment;
struct TheGraph;
struct Storage;


// TODO Ad 2024/06/27: should probably live in a separate preprocessing application/library
// instead of polluting the viewer with asset processing.
// Plus it generally applicable, so a library would make sense.

/// @brief Dump the environment as the 6 faces of a cubemap, as a single image strip.
void dumpEnvironmentCubemap(const Environment & aEnvironment, 
                            const TheGraph & aGraph,
                            Storage & aStorage,
                            std::filesystem::path aOutputStrip);


//class EnvironmentUtilities
//{
//public:
//    EnvironmentUtilities();
//    void filterEnvironmentMap(const Environment & aEnvironment, 
//
//private:
//    graphics::Program mEnvironmentFilter;
//};

} // namespace ad::renderer