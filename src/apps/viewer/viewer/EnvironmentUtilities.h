#pragma once

#include <filesystem>

#include <renderer/Shading.h>
#include <renderer/Texture.h>

#include <snac-renderer-V2/Handle.h>


namespace ad::renderer {


struct Environment;
struct Loader;
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


Handle<graphics::Texture> filterEnvironmentMap(const Environment & aEnvironment,
                                               const TheGraph & aGraph,
                                               Storage & aStorage,
                                               GLsizei aOutputSideLength);


/// @brief Returns a texture showing where `filterEnvironmentMap()` would take its samples
/// @note The function is very limited at the moment, returning a 2D texture and developped
/// only for normals sampling the front face of the environment cubemap.
/// @param aSurfaceNormal The \b right-handed normal for which we take samples 
/// (it will also set the view and reflection)
graphics::Texture highLightSamples(const Environment & aEnvironment,
                                   math::Vec<3, float> aSurfaceNormal,
                                   float aRoughness,
                                   const Loader & aLoader);


// TODO Ad 2024/07/05: It should be moved into a more general header of a library
void dumpToFile(const graphics::Texture & aTexture, std::filesystem::path aOutput, GLint aLevel = 0);


} // namespace ad::renderer