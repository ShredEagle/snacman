#pragma once


// TODO #debugdraw There should be a debug drawer direclty in this lib
#include <snac-renderer-V1/DebugDrawer.h>


namespace ad::renderer {


namespace drawer
{
    constexpr const char * gCamera = "camera";
    constexpr const char * gLight = "light";
    constexpr const char * gRig = "rig";
    constexpr const char * gScene = "scene";
    constexpr const char * gShadow = "shadow";
}

void initializeDebugDrawers();


} // namespace ad::renderer