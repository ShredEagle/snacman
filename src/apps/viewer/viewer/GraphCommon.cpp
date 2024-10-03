#include "GraphCommon.h"


#include <snac-renderer-V2/Camera.h>
#include <snac-renderer-V2/Model.h>
#include <snac-renderer-V2/Semantics.h>
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
}


void loadCameraUbo(const graphics::UniformBufferObject & aUbo, const GpuViewProjectionBlock & aViewProjection)
{
    proto::loadSingle(aUbo, aViewProjection, graphics::BufferHint::DynamicDraw);
}


void loadCameraUbo(const graphics::UniformBufferObject & aUbo, const Camera & aCamera)
{
    loadCameraUbo(aUbo, GpuViewProjectionBlock{aCamera});
}


} // namespace ad::renderer