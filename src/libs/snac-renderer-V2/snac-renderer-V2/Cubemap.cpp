#include "Cubemap.h"

#include "Cube.h"
#include "VertexStreamUtilities.h"
#include "files/Loader.h"

#include <arte/Image.h>


namespace ad::renderer {


graphics::Texture loadCubemapFromStrip(filesystem::path aImageStrip)
{
    arte::Image<math::hdr::Rgb_f> hdrStrip = 
        arte::Image<math::hdr::Rgb_f>::LoadFile(aImageStrip,
                                                arte::ImageOrientation::Unchanged);
    // A requirement for a strip representing the 6 faces of a cubemap
    assert(hdrStrip.height() * 6 == hdrStrip.width());
    GLsizei side = hdrStrip.height();

    graphics::Texture cubemap{GL_TEXTURE_CUBE_MAP};
    allocateStorage(cubemap, GL_RGB16F,
                    side, side,
                    1);

    graphics::ScopedBind bound{cubemap};
    Guard scopedAlignment = graphics::detail::scopeUnpackAlignment((GLint)hdrStrip.rowAlignment());
    for(unsigned int lineIdx = 0; lineIdx != side; ++lineIdx)
    {
        for(unsigned int faceIdx = 0; faceIdx != 6; ++faceIdx)
        {
            glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx,
                0,
                0, lineIdx, side, 1,
                GL_RGB,
                GL_FLOAT,
                hdrStrip.data() + (lineIdx * hdrStrip.width()) + (faceIdx * side));
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //// TODO understand if wrap filtering parameters have an impact on cubemaps
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return cubemap;
}


graphics::Texture loadEquirectangular(filesystem::path aEquirectangularMap)
{
    arte::Image<math::hdr::Rgb_f> hdrMap = 
        arte::Image<math::hdr::Rgb_f>::LoadFile(aEquirectangularMap,
                                                arte::ImageOrientation::Unchanged);

    graphics::Texture equirectMap{GL_TEXTURE_2D};
    graphics::loadImage(equirectMap, hdrMap);

    graphics::ScopedBind bound{equirectMap};
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return equirectMap;
}


//Part makeUnitCube(Storage & aStorage)
//{
//    Handle<VertexStream> vertexStream = primeVertexStream(aStorage);
//
//    AttributeDescription attribute{
//        .mSemantic = semantic::gPosition,
//        .mDimension = 3,
//        .mComponentType = GL_FLOAT,
//    };
//    addVertexAttribute(VertexStream, attribute, vertexBuffer, aVertices.size());
//}


Skybox::Skybox(const Loader & aLoader, Storage & aStorage) :
    mCubeVao{&aStorage.mVaos.emplace_back()},
    mCubemapProgram{aLoader.loadProgram("shaders/Skybox.prog")},
    mEquirectangularProgram{aLoader.loadProgram("shaders/Skybox.prog", {"EQUIRECTANGULAR"})}
{
    std::vector<math::Position<3, float>> vertices;
    vertices.reserve(cube::Maker::gVertexCount);
    for(unsigned int idx = 0; idx != cube::Maker::gVertexCount; ++idx) 
    {
        vertices.push_back(cube::Maker::getPosition(idx));
    }

    graphics::BufferAny & vertexBuffer = aStorage.mBuffers.emplace_back();
    graphics::ScopedBind vboBind{vertexBuffer, graphics::BufferType::Array};
    glBufferData(GL_ARRAY_BUFFER, std::span{vertices}.size_bytes(), vertices.data(), GL_STATIC_DRAW);

    graphics::ScopedBind vaoBind{*mCubeVao};
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
}


} // namespace ad::renderer