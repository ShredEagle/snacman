#pragma once


#include "Mesh.h"

#include <math/Vector.h>

#include <span>


namespace ad {
namespace snac {

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

namespace cube {

    constexpr std::array<math::Position<3, float>, 8> gVertices{
        math::Position<3, float>{-1.0f, -1.0f, -1.0f},
        math::Position<3, float>{-1.0f, -1.0f,  1.0f},
        math::Position<3, float>{-1.0f,  1.0f, -1.0f},
        math::Position<3, float>{-1.0f,  1.0f,  1.0f},

        math::Position<3, float>{ 1.0f, -1.0f, -1.0f},
        math::Position<3, float>{ 1.0f, -1.0f,  1.0f},
        math::Position<3, float>{ 1.0f,  1.0f, -1.0f},
        math::Position<3, float>{ 1.0f,  1.0f,  1.0f},
    };

    constexpr std::array<GLushort, 6*6> gIndices
    {
        // Left
        0, 2, 1,
        1, 2, 3,
        // Front
        1, 3, 5,
        5, 3, 7,
        // Right,
        5, 7, 4,
        4, 7, 6,
        // Back
        4, 6, 0,
        0, 6, 2,
        // Top
        6, 7, 2,
        2, 7, 3,
        // Bottom,
        0, 1, 4,
        4, 1, 5,
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

} // namespace cube

template <class T_vertex>
constexpr std::vector<T_vertex> getExpandedCubeVertices();


template <>
std::vector<math::Position<3, float>> getExpandedCubeVertices()
{
    std::vector<math::Position<3, float>> result;
    result.reserve(36);
    for (int i = 0; i < 36; i++) 
    {
        result.push_back(cube::gVertices[cube::gIndices[i]]);
    }
    return result;
}


struct PositionNormal
{
    math::Position<3, float> position;
    math::Vec<3, float> normal;
};

template <>
std::vector<PositionNormal> getExpandedCubeVertices()
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

//
// Mesh aware methods
//

template <class T_vertex, std::size_t N_extent>
inline graphics::VertexBufferObject loadVBO(std::span<T_vertex, N_extent> aVertices)
{
    graphics::VertexBufferObject vbo;
    //graphics::ScopedBind boundVBO{vbo};
    bind(vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        aVertices.size_bytes(),
        aVertices.data(),
        GL_STATIC_DRAW);
    return vbo;
}


inline Mesh makeCube()
{
    VertexStream geometry;
    // Note: Binding the VAO is not required for loading vertex buffers
    // Make a VBO and load the cube vertices into it
    // Save the index of the next VBO.
    auto index = geometry.mVertexBuffers.size();
    auto vertices = getExpandedCubeVertices<PositionNormal>();
    geometry.mVertexBuffers.push_back({
        .mBuffer = loadVBO(std::span{vertices}),
        .mStride = sizeof(PositionNormal)
    });

    {
        graphics::ClientAttribute position{
            .mDimension = 3,
            .mOffset = offsetof(PositionNormal, position),
            .mDataType = GL_FLOAT
        };
        geometry.mAttributes.emplace(Semantic::Position, VertexStream::Attribute{index, position});
    }
    {
        graphics::ClientAttribute normal{
            .mDimension = 3,
            .mOffset = offsetof(PositionNormal, normal),
            .mDataType = GL_FLOAT
        };
        geometry.mAttributes.emplace(Semantic::Normal, VertexStream::Attribute{index, normal});
    }
    geometry.mVertexCount = static_cast<GLsizei>(vertices.size());

    return Mesh{.mStream = std::move(geometry)};
}


inline Mesh makeTriangle()
{
    VertexStream geometry;
    // Make a VBO and load the cube vertices into it
    auto index = geometry.mVertexBuffers.size();
    std::array<math::Vec<3, float>, 3> vertices = {
        math::Vec<3, float>{-0.6f, -0.4f, 0.f},
        math::Vec<3, float>{ 0.6f, -0.4f, 0.f},
        math::Vec<3, float>{  0.f,  0.6f, 0.f}
    };

    geometry.mVertexBuffers.push_back({
        .mBuffer = loadVBO(std::span{vertices}),
    });
    graphics::ClientAttribute attribute{
        .mDimension = 3,
        .mOffset = 0,
        .mDataType = GL_FLOAT
    };
    geometry.mAttributes.emplace(Semantic::Position, VertexStream::Attribute{index, attribute});
    geometry.mVertexCount = static_cast<GLsizei>(vertices.size());

    return Mesh{.mStream = std::move(geometry)};
}


//float textureBuffer[12 * 6];
//for (int i = 0; i < 36; i++) {
//    textureBuffer[i * 2 + 0] = texCoords[texInds[i % 4]].x;
//    textureBuffer[i * 2 + 1] = texCoords[texInds[i % 4]].y;
//}
//
//float normalBuffer[18 * 6];
//for (int i = 0; i < 36; i++) {
//    normalBuffer[i * 3 + 0] = normals[indices[i / 6]].x;
//    normalBuffer[i * 3 + 1] = normals[indices[i / 6]].y;
//    normalBuffer[i * 3 + 2] = normals[indices[i / 6]].z;
//}

} // namespace snac
} // namespace ad
