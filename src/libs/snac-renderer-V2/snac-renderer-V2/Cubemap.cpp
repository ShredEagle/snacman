#include "Cubemap.h"

#include "Cube.h"
#include "VertexStreamUtilities.h"
#include "files/Loader.h"

#include <arte/Image.h>

#include <graphics/CameraUtilities.h>


namespace ad::renderer {


namespace {

    void setupCubeFiltering()
    {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY, 16.f);

        //// TODO understand if wrap filtering parameters have an impact on cubemaps
        //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    constexpr auto gNegateVertical = math::trans3d::scale(1.f, -1.f, 1.f);
} // unnamed namespace


// Note: OpenGL order of cubemap faces is +X, -X, +Y, -Y, +Z, -Z
// The coordinate system of the cubemap is **left-handed**,
// thus a camera facing -Z in our right-handed world should render the cubemap +Z face.
// (see: https://www.khronos.org/opengl/wiki/Cubemap_Texture#Upload_and_orientation)
// Important: The camera vertical axis is negated (not rotated!) in order to generate images with a top-left origin,
// (i.e. first bytes appearing in the texture correspond to the top row, instead of the bottom row)
// since cubemaps, unlike all other OpenGL textures, are behaving as having a top-left origin.
// (Apparently, this is coming from Renderman,)
const std::array<math::AffineMatrix<4, GLfloat>, 6> gCubeCaptureViewsNegateY{
    graphics::getCameraTransform<GLfloat>({0.f, 0.f, 0.f}, { 1.f,  0.f,  0.f}) * gNegateVertical,
    graphics::getCameraTransform<GLfloat>({0.f, 0.f, 0.f}, {-1.f,  0.f,  0.f}) * gNegateVertical,
    graphics::getCameraTransform<GLfloat>({0.f, 0.f, 0.f}, { 0.f,  1.f,  0.f}, {0.f,  0.f,  1.f}) * gNegateVertical,
    graphics::getCameraTransform<GLfloat>({0.f, 0.f, 0.f}, { 0.f, -1.f,  0.f}, {0.f,  0.f, -1.f}) * gNegateVertical,
    graphics::getCameraTransform<GLfloat>({0.f, 0.f, 0.f}, { 0.f,  0.f, -1.f}) * gNegateVertical, // -Z in our right handed basis
    graphics::getCameraTransform<GLfloat>({0.f, 0.f, 0.f}, { 0.f,  0.f,  1.f}) * gNegateVertical,
};


graphics::Texture loadCubemapFromStrip(filesystem::path aImageStrip)
{
    // Note: Cubemap coordinates are a surprisingly confusing topic
    // (still giving rise to posts such as:
    //  https://community.khronos.org/t/image-orientation-for-cubemaps-actually-a-very-old-topic/105338)
    // My current approach is to load the faces as labeled in the left-handed coordinate system described in:
    // https://www.khronos.org/opengl/wiki/Cubemap_Texture#Upload_and_orientation
    // Yet, the vertical texture coordinate (v) increases from top to bottom on each individual textre image.
    // This is the opposite of the usual bottom origin for texture (u, v) in OpenGL:
    // For this reason, the image files are not flipped vertically as they usually should be.
    // (So, the individual cubemap texture images appear upside down in Nsight Graphics)
    arte::Image<math::hdr::Rgb_f> hdrStrip =
        arte::Image<math::hdr::Rgb_f>::LoadFile(aImageStrip,
                                                arte::ImageOrientation::Unchanged);
    // A requirement for a strip representing the 6 faces of a cubemap
    assert(hdrStrip.height() * 6 == hdrStrip.width());
    const GLsizei side = hdrStrip.height();

    graphics::Texture cubemap{GL_TEXTURE_CUBE_MAP};
    allocateStorage(cubemap,
                    graphics::MappedSizedPixel_v<decltype(hdrStrip)::pixel_format_t>,
                    side, side,
                    graphics::countCompleteMipmaps({side, side}));

    graphics::ScopedBind bound{cubemap};
    Guard scopedAlignment = graphics::detail::scopeUnpackAlignment((GLint)hdrStrip.rowAlignment());
    // Set the stride between source images rows to the strip total width.
    auto scopedRowLength = graphics::detail::scopePixelStorageMode(GL_UNPACK_ROW_LENGTH, hdrStrip.width());
    for(unsigned int faceIdx = 0; faceIdx != 6; ++faceIdx)
    {
        glTexSubImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx,
            0,
            0, 0, side, side,
            GL_RGB,
            GL_FLOAT,
            hdrStrip.data() + (faceIdx * side));
    }

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    setupCubeFiltering();

    return cubemap;
}


graphics::Texture loadCubemapFromSequence(filesystem::path aImageSequence)
{
    const std::filesystem::path parent = aImageSequence.parent_path();
    const std::string stem = aImageSequence.stem().string();
    const std::string extension = aImageSequence.extension().string();

    arte::Image<math::hdr::Rgb_f> hdrFace =
        arte::Image<math::hdr::Rgb_f>::LoadFile(parent / (stem + "_" + std::to_string(0) + extension),
                                                arte::ImageOrientation::Unchanged);
    const GLsizei side = hdrFace.height();

    graphics::Texture cubemap{GL_TEXTURE_CUBE_MAP};
    allocateStorage(cubemap,
                    graphics::MappedSizedPixel_v<decltype(hdrFace)::pixel_format_t>,
                    side, side,
                    1);

    graphics::ScopedBind bound{cubemap};
    Guard scopedAlignment = graphics::detail::scopeUnpackAlignment((GLint)hdrFace.rowAlignment());

    for(unsigned int faceIdx = 0; faceIdx != 6; ++faceIdx)
    {
        if (faceIdx != 0)
        {
            hdrFace =
                arte::Image<math::hdr::Rgb_f>::LoadFile(
                    parent / (stem + "_" + std::to_string(faceIdx) + extension),
                    arte::ImageOrientation::Unchanged);
        }

        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx,
            0,
            0, 0, side, side,
            GL_RGB,
            GL_FLOAT,
            hdrFace.data());
    }

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    setupCubeFiltering();

    return cubemap;
}


graphics::Texture loadEquirectangular(filesystem::path aEquirectangularMap)
{
    arte::Image<math::hdr::Rgb_f> hdrMap =
        arte::Image<math::hdr::Rgb_f>::LoadFile(aEquirectangularMap,
                                                arte::ImageOrientation::InvertVerticalAxis);

    // Mipmapping causes a lot of issues with equirectangular maps:
    // * There is a discontinuity on the sampled v coordinate, at the azimuthal wrap-around
    // * It tends to picks a way to blurry mipmap levels wrt to the actual framebuffer resolution
    //   * TODO: understand why that is the case?
    const GLint mipmapLevels = 1; // 1 means no mipmapping

    graphics::Texture equirectMap{GL_TEXTURE_2D};
    graphics::loadImage(equirectMap, hdrMap, mipmapLevels);

    graphics::ScopedBind bound{equirectMap};
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if(mipmapLevels > 1)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

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
    mEffect{aLoader.loadEffect("effects/Skybox.sefx", aStorage)}
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
