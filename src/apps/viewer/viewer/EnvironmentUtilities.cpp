#include "EnvironmentUtilities.h"

#include "Scene.h"
#include "TheGraph.h"

#include <math/Color.h>
#include <math/Vector.h>

#include <renderer/FrameBuffer.h>
#include <renderer/MappedGL.h>
#include <renderer/Renderbuffer.h>
#include <renderer/ScopeGuards.h>
#include <renderer/Uniforms.h>

#include <snac-renderer-V2/Camera.h>
#include <snac-renderer-V2/Cubemap.h>
#include <snac-renderer-V2/Pass.h> // for getProgram()
#include <snac-renderer-V2/Profiling.h>

#include <snac-renderer-V2/files/Loader.h>


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
        aGraph.passDrawSkybox(aEnvironment, aStorage, GL_BACK);

        glReadPixels(0, 0, renderSize.width(), renderSize.height(),
                     GL_RGB, graphics::MappedPixelComponentType_v<Pixel>, image.data());

        // OpenGL Image origin is bottom left, images on disk are top left, so invert axis.
        image.saveFile(aOutputColumn.parent_path() / (aOutputColumn.stem() += "_" + std::to_string(faceIdx) + ".hdr"), arte::ImageOrientation::Unchanged);
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
        orthographicFace.setPose(gCubeCaptureViewsNegateY[faceIdx]);
        loadCameraUbo(*aGraph.mUbos.mViewingUbo, orthographicFace);

        // When drawing the skybox, we are "inside" the cube, so only render backfaces (cull GL_FRONT)
        // Yet, we use a camera transform that negates the Y coordinates, turning backfaces into frontfaces
        // i.e., we only render frontfaces (cull GL_BACK).
        aGraph.passDrawSkybox(aEnvironment, aStorage, GL_BACK);

        glReadPixels(0, 0, renderSize.width(), renderSize.height(),
                     GL_RGB, graphics::MappedPixelComponentType_v<Pixel>,
                     image.data() + faceIdx * renderSize.width());
    }
    // OpenGL Image origin is bottom left, but cubemaps individual faces origin is actually top-left.
    // Since we directly rendered to the image as a correctly oriented cubemap face 
    // (cf. negated up vector of gCubeCaptureviews),
    // it already has the top-left origin expected by image-file formats.
    image.saveFile(aOutputStrip, arte::ImageOrientation::Unchanged);
#endif
}


namespace {

    graphics::Texture prepareCubemap(const Environment & aEnvironment, math::Size<2, GLsizei> aSize, GLsizei aLevelsCount)
    {
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

        graphics::Texture cubemap{GL_TEXTURE_CUBE_MAP};
        graphics::allocateStorage(cubemap,
                                  environmentInternalFormat,
                                  aSize.width(), aSize.height(),
                                  aLevelsCount);

        return cubemap;
    }
        

    void renderCubemapFaces(const IntrospectProgram & aProgram,
                            const TheGraph & aGraph,
                            const Environment & aEnvironment,
                            Storage & aStorage,
                            const graphics::Texture & aTargetCubemap,
                            GLsizei aLevel)
    {
        // TODO implement as layered rendering instead
        // see: https://www.khronos.org/opengl/wiki/Geometry_Shader#Layered_rendering

        Camera orthographicFace;

        for(unsigned int faceIdx = 0; faceIdx != gFaceCount; ++faceIdx)
        {
            // We attach the current texture level to the Framebuffer's draw color buffer attachment 1 
            // (it could be zero since we do not attach to another color buffer, this is just to be fancy)
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT1,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIdx, 
                                   aTargetCubemap, 
                                   aLevel);

            // Map output fragment color at location 0 to the draw buffer at color attachment 1
            // (this color attachment was set to the output texture face just above)
            glDrawBuffer(GL_COLOR_ATTACHMENT1);

            // Set the appropriate camera pose for this face's pass
            // BUGFIXED: It is required to write the destination cubemap images as having top-left origin
            // This is implemented by negating the Y coordinate of gl_Position (via the provided camera transform)
            orthographicFace.setPose(gCubeCaptureViewsNegateY[faceIdx]);
            loadCameraUbo(*aGraph.mUbos.mViewingUbo, orthographicFace);
            glClear(GL_COLOR_BUFFER_BIT);

            // We need to render cube inner-faces (backfaces), but they are turned into frontfaces by the negated Y camera
            aGraph.passSkyboxBase(aProgram, aEnvironment, aStorage, GL_BACK);
        }
    }

} // unnamed namespace

Handle<graphics::Texture> filterEnvironmentMapSpecular(const Environment & aEnvironment,
                                                       const TheGraph & aGraph,
                                                       Storage & aStorage,
                                                       GLsizei aOutputSideLength)
{
    PROFILER_SCOPE_SINGLESHOT_SECTION(gRenderProfiler, "filter env: specular radiance", CpuTime, GpuTime);

    // Texture level 0 (maximum) size
    const math::Size<2, GLsizei> size{aOutputSideLength, aOutputSideLength};
    const GLint textureLevels = graphics::countCompleteMipmaps(size);
    assert(textureLevels > 1); // otherwise there is just one value for roughness (zero), which is likely wrong
    graphics::Texture filteredCubemap = prepareCubemap(aEnvironment, size, textureLevels);

    graphics::FrameBuffer framebuffer;
    graphics::ScopedBind boundFbo{framebuffer, graphics::FrameBufferTarget::Draw};

    math::Size<2, GLsizei> levelSize = size;

    static const std::vector<Technique::Annotation> annotations{
        {"pass", "prefilter_specular_radiance"},
    };
    Handle<ConfiguredProgram> confProgram = getProgram(*aGraph.mSkybox.mEffect, annotations);

    for(GLint level = 0; level != textureLevels; ++level)
    {
        glViewport(0, 0, levelSize.width(), levelSize.height());

        // I am not 100% sure if level 0 should be roughness 0 or the first step.
        // Roughness zero seems wasteful (I suppose it should be identical to the unfiltered cubemap)
        // but probably more correct to allow mip-levels interpolation.
        float roughness = (float)level / (textureLevels - 1); // Note: we asserted that textueLevels is more than 1
        // TODO: find a more dynamic way to bind those plain uniforms
        graphics::setUniform(confProgram->mProgram, "u_Roughness", roughness);

        renderCubemapFaces(confProgram->mProgram, aGraph, aEnvironment, aStorage, filteredCubemap, level);

        // This is the mipmap size derivation described in: 
        // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexStorage2D.xhtml
        levelSize = max((levelSize / 2), {1, 1});
    }
    
    {
        graphics::ScopedBind boundCubemap{filteredCubemap};
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY, 16.f);
    }

    return &aStorage.mTextures.emplace_back(std::move(filteredCubemap));
}


Handle<graphics::Texture> filterEnvironmentMapDiffuse(const Environment & aEnvironment,
                                                      const TheGraph & aGraph,
                                                      Storage & aStorage,
                                                      GLsizei aOutputSideLength)
{
    PROFILER_SCOPE_SINGLESHOT_SECTION(gRenderProfiler, "filter env: diffuse irradiance", CpuTime, GpuTime);

    // Texture level 0 (maximum) size
    const math::Size<2, GLsizei> size{aOutputSideLength, aOutputSideLength};
    constexpr GLint textureLevels = 1;
    graphics::Texture filteredCubemap = prepareCubemap(aEnvironment, size, textureLevels);

    graphics::FrameBuffer framebuffer;
    graphics::ScopedBind boundFbo{framebuffer, graphics::FrameBufferTarget::Draw};

    static const std::vector<Technique::Annotation> annotations{
        {"pass", "prefilter_diffuse_irradiance"},
    };
    Handle<ConfiguredProgram> confProgram = getProgram(*aGraph.mSkybox.mEffect, annotations);

    glViewport(0, 0, size.width(), size.height());

    constexpr GLint level = 0;
    renderCubemapFaces(confProgram->mProgram, aGraph, aEnvironment, aStorage, filteredCubemap, level);

    {
        graphics::ScopedBind boundCubemap{filteredCubemap};
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    return &aStorage.mTextures.emplace_back(std::move(filteredCubemap));
}


// Note: Render to a texture via the classical framebuffer/viewport approach.
Handle<graphics::Texture> integrateEnvironmentBrdf(Storage & aStorage,
                                                   GLsizei aOutputSideLength,
                                                   const Loader & aLoader)
{
    PROFILER_SCOPE_SINGLESHOT_SECTION(gRenderProfiler, "integrate environment brdf", CpuTime, GpuTime);

    const math::Size<2, GLsizei> size{aOutputSideLength, aOutputSideLength};

    graphics::Texture result{GL_TEXTURE_2D};
    graphics::allocateStorage(result,
                              // Note: the paper recommended 16bit floats for precision
                              GL_RG32F, // This is the commonly used internal format around these functions
                              size.width(), size.height(),
                              1);

    graphics::FrameBuffer framebuffer;
    graphics::ScopedBind boundFbo{framebuffer, graphics::FrameBufferTarget::Draw};

    // We attach the current texture level 0 to the Framebuffer's draw color buffer attachment 1 
    // (it could be zero since we do not attach to another color buffer, this is just to be fancy)
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
                            GL_COLOR_ATTACHMENT1,
                            GL_TEXTURE_2D,
                            result, 
                            0);

    // Map output fragment color at location 2 to the draw buffer at color attachment 1
    // (once again, just to be fancy, we could use the default mapping 
    //  of fragment color at location 0 to the draw buffer at color attachment 1)
    static const std::array<GLenum, 3> drawBuffers{GL_NONE, GL_NONE, GL_COLOR_ATTACHMENT1};
    glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data());

    glViewport(0, 0, size.width(), size.height());

    glClear(GL_COLOR_BUFFER_BIT);

    IntrospectProgram program = aLoader.loadProgram("shaders/IntegrateEnvironmentBrdf.prog");
    graphics::ScopedBind boundProgram(program);

    // Draw the fullscreen quad (which will invoke the FS for each ouptut pixel of the viewport)
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    {
        graphics::ScopedBind boundTexture{result};
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    return &aStorage.mTextures.emplace_back(std::move(result));
}


// Note: Renders with random writes (scatter) to an Image
graphics::Texture highLightSamples(const Environment & aEnvironment,
                                   math::Vec<3, float> aSurfaceNormal,
                                   float aRoughness,
                                   const Loader & aLoader)
{
    PROFILER_SCOPE_SINGLESHOT_SECTION(gRenderProfiler, "hightlight samples", CpuTime, GpuTime);

    assert(aEnvironment.mType == Environment::Cubemap);
    
    // Get the internal format and size of the provided environment texture
    GLint environmentInternalFormat = 0;
    GLint environmentSideSize = 0;
    {
        graphics::ScopedBind boundEnvironment{*aEnvironment.mMap};
        glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X,
                                 0,
                                 GL_TEXTURE_INTERNAL_FORMAT,
                                 &environmentInternalFormat);
        glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X,
                                 0,
                                 GL_TEXTURE_WIDTH,
                                 &environmentSideSize);
    }

    // The initial intention was to be clever and use the same internal format 
    // for our output sample image than for the environment cubempa (so we could composite)
    // Yet, you can't use (most of the) 3-channels image formats with image load/store.
    // see: https://stackoverflow.com/a/74794343/1027706 
    // So, let's hardcode atm:
    GLenum imageFormat = GL_RGBA32F;

    graphics::Texture output{GL_TEXTURE_2D};
    {
        // Only bound for the storage allocation
        graphics::ScopedBind boundTexture{output};

        // Allocate storage for an image (level 0) of the texture.
        // TODO Ad 2024/07/09: Understand why this allocation (pointed out by ChatGPT) does not seem to work
        // TODO Ad 2024/07/09: Find a way to test for texture "completeness", I chased this bug for two hours
        //glTexImage2D(output.mTarget, 0, imageFormat, environmentSideSize, environmentSideSize,
        //             0, GL_RGBA, GL_FLOAT, nullptr);

        glTexStorage2D(output.mTarget, 1, imageFormat, environmentSideSize, environmentSideSize);
    }

    // Bind texture level 0 to image unit 1 (matched by layout binding in fragment shader)
    const GLuint imageUnit = 1;
    glBindImageTexture(imageUnit, output, 0, GL_FALSE, 0, GL_WRITE_ONLY, imageFormat);

    IntrospectProgram program = aLoader.loadProgram("shaders/HighlightSamples.prog");
    graphics::ScopedBind boundProgram(program);

    graphics::setUniform(program, "u_SurfaceNormal", aSurfaceNormal);
    graphics::setUniform(program, "u_Roughness", aRoughness);

    // Rasterizing 1 pixel of the quad is all we need:
    // A single shader invocation takes care of all the job
    // (Note: it would likely be much better to issue 1 sample position per fragment,
    // but this code will not be run in release anyway)
    glViewport(0, 0, 1, 1);

    // We use a FS invocations query to assert it is called exactly once
    GLuint queryId;
    glGenQueries(1, &queryId);
    glBeginQuery(GL_FRAGMENT_SHADER_INVOCATIONS, queryId);

    // I suppose the image is already in state "cleared" after initialization
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glEndQuery(GL_FRAGMENT_SHADER_INVOCATIONS);
    GLuint invocationsResult;
    glGetQueryObjectuiv(queryId, GL_QUERY_RESULT, &invocationsResult);

    assert(invocationsResult == 1);

    // Having them directly here seems too conservative
    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT); // I guess what we need

    return output;
}


void dumpToFile(const graphics::Texture & aTexture, std::filesystem::path aOutput, GLint aLevel)
{
    graphics::ScopedBind boundTexture{aTexture};

    math::Size<2, GLint> size;
    glGetTexLevelParameteriv(aTexture.mTarget,
                             0,
                             GL_TEXTURE_WIDTH,
                             &size.width());
    glGetTexLevelParameteriv(aTexture.mTarget,
                             0,
                             GL_TEXTURE_HEIGHT,
                             &size.height());
    GLint internalFormat;
    glGetTexLevelParameteriv(aTexture.mTarget,
                             0,
                             GL_TEXTURE_INTERNAL_FORMAT,
                             &internalFormat);

    using Pixel = math::sdr::Rgb;

    std::unique_ptr<unsigned char[]> raster = 
        std::make_unique<unsigned char[]>(sizeof(Pixel) * size.area());
    // Return as RGB unsigned bytes
    glGetTexImage(aTexture.mTarget, aLevel, GL_RGB, GL_UNSIGNED_BYTE, raster.get());

    arte::Image<Pixel> result{size, std::move(raster)};
    result.saveFile(aOutput);
}


} // namespace ad::renderer