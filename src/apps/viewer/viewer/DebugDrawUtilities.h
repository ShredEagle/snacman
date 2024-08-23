#pragma once


#include <snac-renderer-V2/debug/DebugDrawing.h>


namespace ad::renderer {



// TODO Ad 2024/08/08: This should be moved to a more general library
/// @brief Debug-draw the view frustum of a camera, provided the inverse of its view-projection matrix.
inline void debugDrawViewFrustum(const math::Matrix<4, 4, float> & aViewProjectionInverse,
                                 const char * aDrawer,
                                 math::hdr::Rgba_f aColor = math::hdr::gMagenta<float>)
{
    std::array<math::Position<4, float>, 8> ndcCorners{{
        {-1.f, -1.f, -1.f, 1.f}, // Near bottom left
        { 1.f, -1.f, -1.f, 1.f}, // Near bottom right
        { 1.f,  1.f, -1.f, 1.f}, // Near top right
        {-1.f,  1.f, -1.f, 1.f}, // Near top left
        {-1.f, -1.f,  1.f, 1.f}, // Far bottom left
        { 1.f, -1.f,  1.f, 1.f}, // Far bottom right
        { 1.f,  1.f,  1.f, 1.f}, // Far top right
        {-1.f,  1.f,  1.f, 1.f}, // Far top left
    }};

    // Transform corners from NDC to world
    for(std::size_t idx = 0; idx != ndcCorners.size(); ++idx)
    {
        ndcCorners[idx] *= aViewProjectionInverse;
    }

    auto drawFace = [&ndcCorners, aDrawer, aColor](const std::size_t aStartIdx)
    {
        for(std::size_t idx = 0; idx != 4; ++idx)
        {
            DBGDRAW_INFO(aDrawer).addLine(
                math::homogeneous::normalize(ndcCorners[idx + aStartIdx]).xyz(),
                math::homogeneous::normalize(ndcCorners[(idx + 1) % 4 + aStartIdx]).xyz(),
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
            math::homogeneous::normalize(ndcCorners[idx]).xyz(),
            math::homogeneous::normalize(ndcCorners[(idx + 4)]).xyz(),
            aColor);
    }
}


} // namespace ad::renderer