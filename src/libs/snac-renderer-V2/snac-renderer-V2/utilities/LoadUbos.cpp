#include "LoadUbos.h"

// TODO Ad 2023/10/18: Should get rid of this repeated implementation
#include "../RendererReimplement.h"

#include "../Camera.h"
#include "../Lights.h"

#include <chrono>


namespace ad::renderer {


void loadFrameUbo(const graphics::UniformBufferObject & aUbo)
{
    GLfloat time =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1000.f;
    proto::loadSingle(aUbo, time, graphics::BufferHint::DynamicDraw);
}


void loadLightsUbo(const graphics::UniformBufferObject & aUbo,
                   const LightsData_glsl & aLights)
{
    proto::loadSingle(aUbo, aLights, graphics::BufferHint::DynamicDraw);
}


void loadCameraUbo(const graphics::UniformBufferObject & aUbo, const GpuViewProjectionBlock & aViewProjection)
{
    proto::loadSingle(aUbo, aViewProjection, graphics::BufferHint::DynamicDraw);
}


void loadCameraUbo(const graphics::UniformBufferObject & aUbo, const Camera & aCamera)
{
    loadCameraUbo(aUbo, GpuViewProjectionBlock{aCamera});
}


void loadLightViewProjectionUbo(const graphics::UniformBufferObject & aUbo, const LightViewProjection & aLightViewProjection)
{
    proto::loadSingle(aUbo, aLightViewProjection, graphics::BufferHint::DynamicDraw);
}


void updateOffsetInLightViewProjectionUbo(const graphics::UniformBufferObject & aUbo,
                                          const LightViewProjection & aLightViewProjection)
{
    graphics::ScopedBind bound{aUbo};
    gl.BufferSubData(
        aUbo.GLTarget_v,
        offsetof(LightViewProjection, mLightViewProjectionOffset),
        sizeof(LightViewProjection::mLightViewProjectionOffset),
        &aLightViewProjection.mLightViewProjectionOffset
    );
}


} // namespace ad::renderer