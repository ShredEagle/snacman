#pragma once


//#include "Mesh.h"

#include <math/Box.h>
#include <math/Vector.h>

#include <span>


// TODO Ad 2024/02/13: Find a way to factorize that and to move it up in a library
// The main design complication is that we want to achieve two "contradictory" goals:
// * Having high level functions so client code can very easily get a cube, or any basic shape.
// * Having generic code that does not force the client code to use any predefined memory layout.

namespace ad::renderer {

// WARNING: this is the intial insipiration, but we did re-organize the vertices in our actual code. 
//constexpr math::Vec<3, float> vertices[8] =
//{
//    {-1., -1., -1.},
//    { 1., -1., -1.},
//    { 1.,  1., -1.},
//    {-1.,  1., -1.},
//    {-1., -1.,  1.},
//    { 1., -1.,  1.},
//    { 1.,  1.,  1.},
//    {-1.,  1.,  1.}
//};
//
//constexpr math::Vec<2, float> texCoords[4] =
//{
//    {0., 0.},
//    {1., 0.},
//    {1., 1.},
//    {0., 1.}
//};
//
//constexpr math::Vec<3, float> normals[6] =
//{
//    { 0.,  0.,  1.},
//    { 1.,  0.,  0.},
//    { 0.,  0., -1.},
//    {-1.,  0.,  0.},
//    { 0.,  1.,  0.},
//    { 0., -1.,  0.}
//};
//
//constexpr std::size_t indices[6 * 6] =
//{
//    0, 1, 3, 3, 1, 2,
//    1, 5, 2, 2, 5, 6,
//    5, 4, 6, 6, 4, 7,
//    4, 0, 7, 7, 0, 3,
//    3, 2, 7, 7, 2, 6,
//    4, 5, 0, 0, 5, 1
//};
//
//constexpr int textureIndices [6] = { 0, 1, 3, 3, 1, 2 };
//
constexpr float gCubeSize = 2.f;

namespace cube {

    // The cube from [-1, -1, -1] to [1, 1, 1]
    constexpr std::array<math::Position<3, float>, 8> gVertices{
        math::Position<3, float>{-gCubeSize / 2, -gCubeSize / 2, -gCubeSize / 2},
        math::Position<3, float>{-gCubeSize / 2, -gCubeSize / 2,  gCubeSize / 2},
        math::Position<3, float>{-gCubeSize / 2,  gCubeSize / 2, -gCubeSize / 2},
        math::Position<3, float>{-gCubeSize / 2,  gCubeSize / 2,  gCubeSize / 2},

        math::Position<3, float>{ gCubeSize / 2, -gCubeSize / 2, -gCubeSize / 2},
        math::Position<3, float>{ gCubeSize / 2, -gCubeSize / 2,  gCubeSize / 2},
        math::Position<3, float>{ gCubeSize / 2,  gCubeSize / 2, -gCubeSize / 2},
        math::Position<3, float>{ gCubeSize / 2,  gCubeSize / 2,  gCubeSize / 2},
    };

    constexpr std::array<GLushort, 6*6> gIndices
    {
        // Left
        0, 1, 2,
        2, 1, 3,
        // Front
        1, 5, 3,
        3, 5, 7,
        // Right,
        5, 4, 7,
        7, 4, 6,
        // Back
        4, 0, 6,
        6, 0, 2,
        // Top
        6, 2, 7,
        7, 2, 3,
        // Bottom,
        0, 4, 1,
        1, 4, 5,
    };

    constexpr std::array<math::Vec<3, float>, 6> gNormals =
    {
        // Left
        math::Vec<3, float>{-1.f,  0.f,  0.f},
        //Front
        math::Vec<3, float>{ 0.f,  0.f,  1.f},
        // Right
        math::Vec<3, float>{ 1.f,  0.f,  0.f},
        // Back
        math::Vec<3, float>{ 0.f,  0.f, -1.f},
        // Top
        math::Vec<3, float>{ 0.f,  1.f,  0.f},
        // Bottom
        math::Vec<3, float>{ 0.f, -1.f,  0.f},
    };

    inline std::array<math::Position<3, float>, 8> getBoxVertices(math::Box<float> aBox)
    {
        return {
            aBox.bottomLeftBack(),
            aBox.bottomLeftFront(),
            aBox.topLeftBack(),
            aBox.topLeftFront(),

            aBox.bottomRightBack(),
            aBox.bottomRightFront(),
            aBox.topRightBack(),
            aBox.topRightFront(),
        };
    }

} // namespace cube


namespace triangle {

    constexpr std::array<math::Position<3, float>, 3> gPositions{
        math::Position<3, float>{ 1.f, -1.f, 0.f},
        math::Position<3, float>{ 0.f,  1.f, 0.f},
        math::Position<3, float>{-1.f, -1.f, 0.f},
    };

    struct Maker
    {
        static constexpr unsigned int gVertexCount = 3;

        static math::Position<3, float> getPosition(unsigned int aIdx)
        {
            return gPositions[aIdx];
        }

        static math::Vec<3, float> getNormal(unsigned int aIdx)
        {
            return {0.f, 0.f, 1.f};
        }
    };

} // namespace triangle


inline std::vector<math::Position<3, float>> getExpandedCubePositions()
{
    std::vector<math::Position<3, float>> result;
    result.reserve(36);
    for (int i = 0; i < 36; i++) 
    {
        result.push_back(cube::gVertices[cube::gIndices[i]]);
    }
    return result;
}


inline std::vector<math::Vec<3, float>> getExpandedCubeNormals()
{
    std::vector<math::Vec<3, float>> result;
    result.reserve(36);
    for (int i = 0; i < 36; i++) 
    {
        result.push_back(cube::gNormals[i / 6]);
    }
    return result;
}


template <class T_vertex>
constexpr std::vector<T_vertex> getExpandedCubeVertices();


template <>
inline std::vector<math::Position<3, float>> getExpandedCubeVertices()
{
    return getExpandedCubePositions();
}

inline const std::array<math::Position<3, float>, 10> & getArrowVertices()
{
    using Pos = math::Position<3, float>;
    static std::array<Pos, 10> gVertices{
        // Tail
        Pos{0.f, 0.f, 0.f},
        Pos{0.f, 1.f, 0.f},
        // Heads
        Pos{0.f, 1.f, 0.f},
        Pos{0.f, 0.75f, 0.15f},

        Pos{0.f, 1.f, 0.f},
        Pos{0.f, 0.75f, -0.15f},

        Pos{0.f, 1.f, 0.f},
        Pos{0.15f, 0.75f, 0.f},

        Pos{0.f, 1.f, 0.f},
        Pos{-0.15f, 0.75f, 0.f},
    };
    return gVertices;
}


//  TODO use PositionNormalUV instead
struct PositionNormal
{
    math::Position<3, float> position;
    math::Vec<3, float> normal;
};

template <>
inline std::vector<PositionNormal> getExpandedCubeVertices()
{
    std::vector<PositionNormal> result;
    result.reserve(36);
    for (int i = 0; i < 36; i++) 
    {
        result.push_back(PositionNormal{
            .position = cube::gVertices[cube::gIndices[i]],
            .normal = cube::gNormals[i / 6]
        });
    }
    return result;
}


inline std::vector<PositionNormal> getExpandedVertices(math::Box<float> aBox)
{
    std::vector<PositionNormal> result;
    result.reserve(36);
    auto vertices = cube::getBoxVertices(aBox);
    for (int i = 0; i < 36; i++) 
    {
        result.push_back(PositionNormal{
            .position = vertices[cube::gIndices[i]],
            .normal = cube::gNormals[i / 6]
        });
    }
    return result;
}


////
//// Mesh aware methods
////
//
//template <class T_vertex, std::size_t N_extent>
//inline graphics::VertexBufferObject loadVBO(std::span<T_vertex, N_extent> aVertices)
//{
//    graphics::VertexBufferObject vbo;
//    graphics::ScopedBind boundVBO{vbo};
//    glBufferData(
//        GL_ARRAY_BUFFER,
//        aVertices.size_bytes(),
//        aVertices.data(),
//        GL_STATIC_DRAW);
//    return vbo;
//}


} // namespace ad::renderer
