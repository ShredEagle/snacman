#include "EnvironmentMapping.h"

#include "../Cubemap.h"
#include "../Profiling.h"
#include "../Semantics.h"

#include "../files/Loader.h"

#include "../utilities/VertexStreamUtilities.h"


namespace ad::renderer {


Environment loadEnvironmentMap(const std::filesystem::path aDdsPath, Storage & aStorage)
{
    PROFILER_SCOPE_SINGLESHOT_SECTION(gRenderProfiler, "Load environment map",
                                      CpuTime, GpuTime, BufferMemoryWritten);

    return Environment{
        .mTextureRepository{ 
            {
                semantic::gEnvironmentTexture, 
                pushTexture(aStorage, loadCubemapFromDds(aDdsPath), "environment_map"),
            },
        },
    };
}


SkyPassCache::SkyPassCache(const Loader & aLoader, Storage & aStorage)
{
    Handle<ConfiguredProgram> program =
        storeConfiguredProgram(aLoader.loadProgram("programs/SkyboxNew.prog"),
                               aStorage);

    mPassCache = PassCache{
        .mCalls{
            DrawCall{
                // TODO Ad 2024/12/12: There should be a consolidate method to get the DrawCall,
                // but this will overlap with DrawEntryHelper.
                .mPrimitiveMode = GL_TRIANGLE_STRIP,
                .mIndicesType = GL_NONE,
                // TODO: this seems unsafe regarding lifetime
                .mProgram = &program->mProgram,
                .mVao = aStorage.mDummyVao,
                .mDrawCount = 1,
            },
        },
        .mDrawCommands{
            DrawElementsIndirectCommand{
                .mCount = 14,
                .mInstanceCount = 1,
                // TODO Ad 2024/12/12: this is plain wrong, we should use the correct type of command for array
                // see: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMultiDrawArraysIndirect.xhtml
                // This works because there is only one command.
                .mFirstIndex = 0,
                .mBaseVertex = 0,
                .mBaseInstance = 0,
            },
        }
    };
}


void SkyPassCache::pass(const RepositoryUbo & aUboRepository, const Environment & aEnvironment)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "pass_skybox", CpuTime, GpuTime);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glEnable(GL_CULL_FACE);
    auto scopedCullFace = graphics::scopeCullFace(GL_FRONT);

    draw(mPassCache, aUboRepository, aEnvironment.mTextureRepository);
}


} // namespace ad::renderer