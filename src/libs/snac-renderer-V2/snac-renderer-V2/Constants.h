#pragma once


#include <renderer/commons.h>

#include <string>


namespace ad::renderer {


constexpr unsigned int gMaxEntities = 512;
constexpr unsigned int gMaxJoints   = 512;


// Lights
constexpr unsigned int gMaxLights = 16;
constexpr unsigned int gMaxShadowLights = 4;
constexpr unsigned int gCascadesPerShadow = 4;
constexpr unsigned int gMaxShadowMaps = gMaxShadowLights * gCascadesPerShadow;

// The SDF spread used in FreeType sdf renderer.
// It means the distance, in texels from the edge (==128) to extreme values.
// see: https://freetype.org/freetype2/docs/reference/ft2-properties.html#spread
constexpr unsigned int gSdfSpread = 8;


const std::vector<graphics::MacroDefine> gClientConstantDefines{
    "CLIENT_MAX_ENTITIES " + std::to_string(gMaxEntities),
    "CLIENT_MAX_JOINTS " + std::to_string(gMaxJoints),
    "CLIENT_SDF_DOUBLE_SPREAD " + std::to_string(2 * gSdfSpread),
    "CLIENT_MAX_LIGHTS " + std::to_string(gMaxLights),
    "CLIENT_MAX_SHADOW_LIGHTS " + std::to_string(gMaxShadowLights),
    "CLIENT_CASCADES_PER_SHADOW " + std::to_string(gCascadesPerShadow),
};


} // namespace ad::renderer