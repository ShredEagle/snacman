#pragma once


namespace ad::renderer {


constexpr unsigned int gVertexPosition = 0b1;
constexpr unsigned int gVertexNormal = gVertexPosition << 1;
constexpr unsigned int gVertexTangent = gVertexPosition << 2;
constexpr unsigned int gVertexBitangent = gVertexPosition << 3;

inline bool has(unsigned int aFlags, unsigned int aTag)
{
    return (aFlags & aTag) == aTag;
}

} // namespace ad::renderer