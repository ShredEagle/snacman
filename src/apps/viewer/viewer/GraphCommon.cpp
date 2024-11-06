#include "GraphCommon.h"

#include "PassViewer.h"

#include <snac-renderer-V2/Camera.h>
#include <snac-renderer-V2/Model.h>
#include <snac-renderer-V2/Lights.h>
#include <snac-renderer-V2/Profiling.h>
#include <snac-renderer-V2/Semantics.h>

#include <snac-renderer-V2/files/Loader.h>

// TODO Ad 2023/10/18: Should get rid of this repeated implementation
#include <snac-renderer-V2/RendererReimplement.h>


namespace ad::renderer {


//
// HardcodedUbos
//
HardcodedUbos::HardcodedUbos(Storage & aStorage)
{
    aStorage.mUbos.emplace_back();
    mFrameUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gFrame, mFrameUbo);

    aStorage.mUbos.emplace_back();
    mViewingUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gViewProjection, mViewingUbo);

    aStorage.mUbos.emplace_back();
    mModelTransformUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gLocalToWorld, mModelTransformUbo);

    aStorage.mUbos.emplace_back();
    mJointMatrixPaletteUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gJointMatrices, mJointMatrixPaletteUbo);

    aStorage.mUbos.emplace_back();
    mLightsUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gLights, mLightsUbo);

    aStorage.mUbos.emplace_back();
    mLightViewProjectionUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gLightViewProjection, mLightViewProjectionUbo);

    aStorage.mUbos.emplace_back();
    mShadowCascadeUbo = &aStorage.mUbos.back();
    mUboRepository.emplace(semantic::gShadowCascade, mShadowCascadeUbo);
}


GraphShared::GraphShared(Loader & aLoader, Storage & aStorage) :
    mUbos{aStorage},
    mInstanceStream{makeInstanceStream(aStorage, gMaxDrawInstances)},
    mSkybox{aLoader, aStorage}
{}


// TODO Ad 2023/09/26: Could be splitted between the part list load (which should be valid accross passes)
// and the pass cache load (which might only be valid for a single pass)
void loadDrawBuffers(const GraphShared & aGraphShared, const ViewerPassCache & aPassCache)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "load_draw_buffers", CpuTime, BufferMemoryWritten);

    assert(aPassCache.mDrawInstances.size() <= gMaxDrawInstances);

    // TODO Ad 2023/11/23: this hardcoded access to first buffer view is ugly
    proto::load(*aGraphShared.mInstanceStream.mVertexBufferViews.at(0).mGLBuffer,
                std::span{aPassCache.mDrawInstances},
                graphics::BufferHint::DynamicDraw);

    proto::load(aGraphShared.mIndirectBuffer,
                std::span{aPassCache.mDrawCommands},
                graphics::BufferHint::DynamicDraw);
}


} // namespace ad::renderer