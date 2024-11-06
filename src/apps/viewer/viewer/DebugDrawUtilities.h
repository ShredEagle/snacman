#pragma once


#include "FrustumUtilities.h"

#include <snac-renderer-V2/debug/DebugDrawing.h>


namespace ad::renderer {


inline void debugDrawTriangle(const Triangle & aTri, math::hdr::Rgb_f aColor = math::hdr::gGreen<GLfloat>)
{
    DBGDRAW_INFO(drawer::gShadow).addLine(
        aTri[0].xyz(), aTri[1].xyz(),
        aColor);
    DBGDRAW_INFO(drawer::gShadow).addLine(
        aTri[0].xyz(), aTri[2].xyz(),
        aColor);
    DBGDRAW_INFO(drawer::gShadow).addLine(
        aTri[1].xyz(), aTri[2].xyz(),
        aColor);
}


// TODO Ad 2024/08/08: This should be moved to a more general library
/// @brief Debug-draw the view frustum of a camera, provided the inverse of its view-projection matrix.
inline void debugDrawViewFrustum(const math::Matrix<4, 4, float> & aViewProjectionInverse,
                                 const char * aDrawer,
                                 math::hdr::Rgba_f aColor = math::hdr::gMagenta<float>)
{
    std::array<math::Position<4, float>, 8> corners_worldSpace;

    // Transform corners from NDC to world
    for(std::size_t idx = 0; idx != corners_worldSpace.size(); ++idx)
    {
        corners_worldSpace[idx] = gNdcUnitCorners[idx] * aViewProjectionInverse;
    }

    auto drawFace = [&corners_worldSpace, aDrawer, aColor](const std::size_t aStartIdx)
    {
        for(std::size_t idx = 0; idx != 4; ++idx)
        {
            DBGDRAW_INFO(aDrawer).addLine(
                math::homogeneous::homogenize(corners_worldSpace[idx + aStartIdx]).xyz(),
                math::homogeneous::homogenize(corners_worldSpace[(idx + 1) % 4 + aStartIdx]).xyz(),
                aColor);
        }
    };

    // Near plane
    drawFace(0);
    // Far plane
    drawFace(4);
    // Sides
    for(std::size_t idx = 0; idx != 4; ++idx)
    {
        DBGDRAW_INFO(aDrawer).addLine(
            math::homogeneous::homogenize(corners_worldSpace[idx]).xyz(),
            math::homogeneous::homogenize(corners_worldSpace[(idx + 4)]).xyz(),
            aColor);
    }
}


} // namespace ad::renderer