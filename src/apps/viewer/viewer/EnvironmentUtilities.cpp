#include "EnvironmentUtilities.h"

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


//EnvironmentUtilities::EnvironmentUtilities() :
//    mEnvironmentFilter{}
//{
//
//}

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
    // Configure the framebuffer to output fragment color 1 to its color attachment 1
    // (see: https://www.khronos.org/opengl/wiki/Fragment_Shader#Output_buffers)
    const std::array<GLenum, 2> colorBuffers{GL_NONE, GL_COLOR_ATTACHMENT1};
    glDrawBuffers((GLsizei)colorBuffers.size(), colorBuffers.data()); // This is FB state
    // Set the color buffer at attachment 1 as the source for subsequent read operations
    glReadBuffer(GL_COLOR_ATTACHMENT1);

    Camera orthographicFace;
    constexpr unsigned int gFaceCount = 6;

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
        aGraph.passSkybox(aEnvironment, aStorage);

        glReadPixels(0, 0, renderSize.width(), renderSize.height(),
                     GL_RGB, graphics::MappedPixelComponentType_v<Pixel>,
                     image.data() + faceIdx * renderSize.width());
    }
    // OpenGL Image origin is bottom left, images on disk are top left, so invert axis.
    image.saveFile(aOutputStrip, arte::ImageOrientation::InvertVerticalAxis);
#endif
}


} // namespace ad::renderer