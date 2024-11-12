#include "Passes.h"

#include "../PassViewer.h"
#include "../Scene.h"

#include <profiler/GlApi.h>

#include <renderer/Uniforms.h>

#include <snac-renderer-V2/Pass.h>
#include <snac-renderer-V2/Profiling.h>


namespace ad::renderer {


void passOpaqueDepth(const GraphShared & aGraphShared,
                     const ViewerPartList & aPartList,
                     const RepositoryTexture & aTextureRepository,
                     Storage & aStorage,
                     DepthMethod aMethod)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "pass_depth", CpuTime, GpuTime);

    auto annotations =
        aMethod == DepthMethod::Cascaded ?
            selectPass("cascaded_depth_opaque")
            : selectPass("depth_opaque");
    
    // Can be done once even for distinct cameras, if there is no culling
    ViewerPassCache passCache = prepareViewerPass(annotations , aPartList, aStorage);

    // Load the data for the part and pass related UBOs (TODO: SSBOs)
    loadDrawBuffers(aGraphShared, passCache);

    //
    // Set pipeline state
    //
    {
        PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "set_pipeline_state", CpuTime);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

    }

    // Might loop over cameras, or any other variation
    //for(whatever dimension)
    {
        {
            PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "draw_instances", CpuTime, GpuTime, GpuPrimitiveGen, DrawCalls, BufferMemoryWritten);
            draw(passCache, aGraphShared.mUbos.mUboRepository, aTextureRepository);
        }
    }
}



void passForward(const GraphShared & aGraphShared,
                 const ViewerPartList & aPartList,
                 const RepositoryTexture & aTextureRepository,
                 Storage & aStorage,
                 const GraphControls aControls,
                 bool aCascadedShadowMap,
                 bool aEnvironmentMapping)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "pass_forward", CpuTime, GpuTime);

    // Can be done once for distinct camera, if there is no culling
    std::vector<Technique::Annotation> annotations{
        {"pass", *aControls.mForwardPassKey},
    };
    if(aEnvironmentMapping)
    {
        annotations.push_back({"environment", "on"});
    }

    if(aCascadedShadowMap)
    {
        annotations.push_back({"csm", "on"});
    }

    ViewerPassCache passCache = prepareViewerPass(annotations, aPartList, aStorage);

    // Load the data for the part and pass related UBOs (TODO: SSBOs)
    loadDrawBuffers(aGraphShared, passCache);

    //
    // Set pipeline state
    //
    {
        PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "set_pipeline_state", CpuTime);
        // TODO handle pipeline state with an abstraction
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        // We implemented alpha testing (cut-out), no blending.
        glDisable(GL_BLEND);

    }
    auto scopedPolygonMode = graphics::scopePolygonMode(*aControls.mForwardPolygonMode);

    // Might loop over cameras, or any other variation
    // (but this would imply to move glViewport call back inside this loop)
    //for(whatever dimension)
    {
        // TODO should be done once per viewport
        //glViewport(0, 0, mRenderSize.width(), mRenderSize.height());

        // Reset texture bindings (to make sure that texturing does not work by "accident")
        // this could be extended to reset everything in the context.
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        {
            PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "draw_instances", CpuTime, GpuTime, GpuPrimitiveGen, DrawCalls, BufferMemoryWritten);
            draw(passCache, aGraphShared.mUbos.mUboRepository, aTextureRepository);
        }
    }
}


// TODO: rewrite using our model and abstractions
void passSkyboxBase(const GraphShared & aGraphShared,
                    const IntrospectProgram & aProgram,
                    const Environment & aEnvironment,
                    Storage & aStorage,
                    GLenum aCulledFace,
                    const GraphControls aControls)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "pass_skybox", CpuTime, GpuTime);

    glUseProgram(aProgram);

    Handle<const graphics::Texture> envMap = 
        aEnvironment.mTextureRepository.at(semantic::gEnvironmentTexture);

    glBindVertexArray(*aGraphShared.mSkybox.mCubeVao);
    glActiveTexture(GL_TEXTURE5);
    switch(aEnvironment.mType)
    {
        case Environment::Cubemap:
            glBindTexture(GL_TEXTURE_CUBE_MAP, *envMap);
            break;
        case Environment::Equirectangular:
            glBindTexture(GL_TEXTURE_2D, *envMap);
            break;
    }

    // TODO Ad 2024/06/29: Use the existing setup code to bind texture and UBOs
    graphics::setUniform(aProgram, "u_SkyboxTexture", 5);

    // This is bad, harcoding knowledge of the binding points
    // but this was written as a refresher on native GL
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, *aGraphShared.mUbos.mViewingUbo);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glEnable(GL_CULL_FACE);
    auto scopedCullFace = graphics::scopeCullFace(aCulledFace);

    auto scopedPolygonMode = graphics::scopePolygonMode(*aControls.mForwardPolygonMode);

    glDrawArrays(GL_TRIANGLES, 0, 36);
}


void passDrawSkybox(const GraphShared & aGraphShared,
                    const Environment & aEnvironment,
                    Storage & aStorage,
                    GLenum aCulledFace,
                    const GraphControls aControls)
{
    std::vector<Technique::Annotation> annotations{
        {"pass", "forward"},
        {"environment_texture", to_string(aEnvironment.mType)},
    };
    Handle<ConfiguredProgram> confProgram = getProgram(*aGraphShared.mSkybox.mEffect, annotations);

    passSkyboxBase(aGraphShared, confProgram->mProgram, aEnvironment, aStorage, aCulledFace, aControls);
}



} // namespace ad::renderer