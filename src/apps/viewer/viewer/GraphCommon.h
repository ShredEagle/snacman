#pragma once


#include <snac-renderer-V2/Repositories.h>


namespace ad::renderer {


class Camera;
struct GpuViewProjectionBlock;
struct Storage;


struct HardcodedUbos
{
    HardcodedUbos(Storage & aStorage);

    RepositoryUbo mUboRepository;
    graphics::UniformBufferObject * mFrameUbo;
    graphics::UniformBufferObject * mViewingUbo;
    graphics::UniformBufferObject * mModelTransformUbo;

    graphics::UniformBufferObject * mJointMatrixPaletteUbo;
    graphics::UniformBufferObject * mLightsUbo;
    graphics::UniformBufferObject * mLightViewProjectionUbo;
};


/// @brief Load data from aCamera into aUbo.
/// @note It is proving useful to have access to it to re-use passes outside of the main renderFrame()
void loadCameraUbo(const graphics::UniformBufferObject & aUbo, const Camera & aCamera);

void loadCameraUbo(const graphics::UniformBufferObject & aUbo, const GpuViewProjectionBlock & aViewProjection);


} // namespace ad::renderer