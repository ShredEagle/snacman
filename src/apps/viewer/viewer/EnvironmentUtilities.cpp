#include "EnvironmentUtilities.h"

#include "Scene.h"
#include "TheGraph.h"

#include <math/Color.h>
#include <math/Vector.h>

#include <renderer/FrameBuffer.h>
#include <renderer/MappedGL.h>
#include <renderer/Renderbuffer.h>
#include <renderer/ScopeGuards.h>

#include <snac-renderer-V2/Camera.h>
#include <snac-renderer-V2/Cubemap.h>
#include <snac-renderer-V2/Profiling.h>


namespace ad::renderer {


namespace {
    constexpr unsigned int gFaceCount = 6;
} // unnamed namespace


void dumpEnvironmentCubemap(const Environment & aEnvironment, 
                            const TheGraph & aGraph,
                            Storage & aStorage,
                            std::filesystem::path aOutputStrip)
{
    PROFILER_SCOPE_SINGLESHOT_SECTION(gRenderProfiler, "dump environment cubemap", CpuTime, GpuTime);

    using Pixel = math::hdr::Rgb_f;

    graphics::Renderbuffer renderbuffer;
    graphics::ScopedBind boundRenderbuffer{renderbuffer};

    const math::Size<2, GLsizei> renderSize{2048, 2048};
    glRenderbufferStorage(
        GL_RENDERBUFFER, 
        graphics::MappedSizedPixel_v<Pixel>,
        renderSize.width(), renderSize.height());

    graphics::FrameBuffer framebuffer;
    // Bound as both READ and DRAW framebuffer
    graphics::ScopedBind boundFbo{framebuffer};
    glViewport(0, 0, renderSize.width(), renderSize.height());
    // Attach the renderbuffer to color attachment 1
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, renderbuffer);
    // Configure the fragment shader color output at location 1
    // to write to the Framebuffer's draw buffer color attachment 1,
    // and disable output to the fragment color at location 0
    // (see: https://www.khronos.org/opengl/wiki/Fragment_Shader#Output_buffers)
    const std::array<GLenum, 2> colorBuffers{GL_NONE, GL_COLOR_ATTACHMENT1};
    glDrawBuffers((GLsizei)colorBuffers.size(), colorBuffers.data()); // This is FB state
    // Set the color buffer at attachment 1 as the source for subsequent read operations
    glReadBuffer(GL_COLOR_ATTACHMENT1);

    Camera orthographicFace;

#if defined(DUMP_INDIVIDUAL_IMAGES)
    std::unique_ptr<unsigned char[]> raster = std::make_unique<unsigned char[]>(sizeof(Pixel) * renderSize.area());
    arte::Image<Pixel> image{renderSize, std::move(raster)};

    // Render the skybox
    for(unsigned int faceIdx = 0; faceIdx != gFaceCount; ++faceIdx)
    {
        // Make a pass with the appropriate camera pose
        orthographicFace.setPose(gCubeCaptureViews[faceIdx]);
        loadCameraUbo(*aGraph.mUbos.mViewingUbo, orthographicFace);
        aGraph.passSkybox(aEnvironment, aStorage);

        glReadPixels(0, 0, renderSize.width(), renderSize.height(),
                     GL_RGB, graphics::MappedPixelComponentType_v<Pixel>, image.data());

        // OpenGL Image origin is bottom left, images on disk are top left, so invert axis.
        image.saveFile(aOutputColumn.parent_path() / (aOutputColumn.stem() += "_" + std::to_string(faceIdx) + ".hdr"), arte::ImageOrientation::InvertVerticalAxis);
    }
#else
    std::unique_ptr<unsigned char[]> raster = std::make_unique<unsigned char[]>(sizeof(Pixel) * renderSize.area() * gFaceCount);
    arte::Image<Pixel> image{renderSize.cwMul({(GLsizei)gFaceCount, 1}), std::move(raster)};

    // Set the stride between consecutive rows of pixel to be the image width
    auto scopedRowLength = graphics::detail::scopePixelStorageMode(GL_PACK_ROW_LENGTH, image.width());

    // Render the skybox
    for(unsigned int faceIdx = 0; faceIdx != gFaceCount; ++faceIdx)
    {
        // Make a pass with the appropriate camera pose
        orthographicFace.setPose(gCubeCaptureViews[faceIdx]);
        loadCameraUbo(*aGraph.mUbos.mViewingUbo, orthographicFace);
        aGraph.passDrawSkybox(aEnvironment, aStorage);

        glReadPixels(0, 0, renderSize.width(), renderSize.height(),
                     GL_RGB, graphics::MappedPixelComponentType_v<Pixel>,
                     image.data() + faceIdx * renderSize.width());
    }
    // OpenGL Image origin is bottom left, images on disk are top left, so invert axis.
    image.saveFile(aOutputStrip, arte::ImageOrientation::InvertVerticalAxis);
#endif
}


Handle<graphics::Texture> filterEnvironmentMap(const Environment & aEnvironment,
                                               const TheGraph & aGraph,
                                               Storage & aStorage,
                                               GLsizei aOutputSideLength)
{
    PROFILER_SCOPE_SINGLESHOT_SECTION(gRenderProfiler, "filter environment map", CpuTime, GpuTime);

    // Texture level 1 (maximum) size
    const math::Size<2, GLsizei> size{aOutputSideLength, aOutputSideLength};
    const GLint textureLevels = graphics::countCompleteMipmaps(size);
    assert(textureLevels > 1); // otherwise there is just one value for roughness (zero), which is likely wrong
    
    // We could actually hande equirectangular, but it will also require extension of the filtering shader
    assert(aEnvironment.mType == Environment::Cubemap);
    
    // Get the internal format of the provided environment texture
    GLint environmentInternalFormat = 0;
    {
        graphics::ScopedBind boundEnvironment{*aEnvironment.mMap};
        glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X,
                                 0,
                                 GL_TEXTURE_INTERNAL_FORMAT,
                                 &environmentInternalFormat);
    }


    graphics::Texture filteredCubemap{GL_TEXTURE_CUBE_MAP};
    graphics::allocateStorage(filteredCubemap,
                              environmentInternalFormat,
                              aOutputSideLength, aOutputSideLength,
                              textureLevels);

    graphics::FrameBuffer framebuffer;
    graphics::ScopedBind boundFbo{framebuffer, graphics::FrameBufferTarget::Draw};

    // TODO implement as layered rendering instead
    // see: https://www.khronos.org/opengl/wiki/Geometry_Shader#Layered_rendering

    Camera orthographicFace;

    math::Size<2, GLsizei> levelSize = size;
    for(GLint level = 0; level != textureLevels; ++level)
    {
        // I am not 100% sure if level 1 should be roughness 0 or the first step.
        // Roughness zero seems wasteful (I suppose it should be identical to the unfiltered cubemap)
        // but probably more correct.
        float roughness = (float)level / (textureLevels - 1); // Note: we asserted that textueLevels is more than 1
        float alphaSquared = std::pow(roughness, 4.f);

        glViewport(0, 0, levelSize.width(), levelSize.height());

        for(unsigned int faceIdx = 0; faceIdx != gFaceCount; ++faceIdx)
        {
            // We attach the current texture level to the Framebuffer's draw color buffer attachment 1 
            // (it could be zero since we do not attach to another color buffer, this is just to be fancy)
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT1,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx, 
                                   filteredCubemap, 
                                   level);

            // Map output fragment color at location 0 to the draw buffer at color attachment 1
            // (this color attachment was set to the output texture face just above)
            glDrawBuffer(GL_COLOR_ATTACHMENT1);

            // Set the appropriate camera pose for this face's pass
            orthographicFace.setPose(gCubeCaptureViews[faceIdx]);
            loadCameraUbo(*aGraph.mUbos.mViewingUbo, orthographicFace);
            glClear(GL_COLOR_BUFFER_BIT);
            aGraph.passFilterEnvironment(aEnvironment, alphaSquared, aStorage);
        }

        // This is the mipmap size derivation described in: 
        // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexStorage2D.xhtml
        levelSize = max((levelSize / 2), {1, 1});
    }

    return &aStorage.mTextures.emplace_back(std::move(filteredCubemap));
}


} // namespace ad::renderer