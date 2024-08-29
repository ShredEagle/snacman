#pragma once


namespace ad::renderer {


// The corners of the (OpenGL) unit-cube in homogeneous clip-space, after perspective division.
// Their xyz correspond to the corners in NDC.
constexpr std::array<math::Position<4, float>, 8> gNdcUnitCorners{{
    {-1.f, -1.f, -1.f, 1.f}, // Near bottom left
    { 1.f, -1.f, -1.f, 1.f}, // Near bottom right
    { 1.f,  1.f, -1.f, 1.f}, // Near top right
    {-1.f,  1.f, -1.f, 1.f}, // Near top left
    {-1.f, -1.f,  1.f, 1.f}, // Far bottom left
    { 1.f, -1.f,  1.f, 1.f}, // Far bottom right
    { 1.f,  1.f,  1.f, 1.f}, // Far top right
    {-1.f,  1.f,  1.f, 1.f}, // Far top left
}};


} // namespace ad::renderer