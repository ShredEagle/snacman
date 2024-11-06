#pragma once


#include <renderer/UniformBuffer.h>


namespace ad::renderer {


class Camera;
struct GpuViewProjectionBlock;
struct LightsData_glsl;
struct LightViewProjection;


void loadFrameUbo(const graphics::UniformBufferObject & aUbo);


void loadLightsUbo(const graphics::UniformBufferObject & aUbo, const LightsData_glsl & aLights);


/// @brief Load data from aCamera into aUbo.
/// @note It is proving useful to have access to it to re-use passes outside of the main renderFrame()
void loadCameraUbo(const graphics::UniformBufferObject & aUbo,
                   const Camera & aCamera);

void loadCameraUbo(const graphics::UniformBufferObject & aUbo,
                   const GpuViewProjectionBlock & aViewProjection);

void loadLightViewProjectionUbo(const graphics::UniformBufferObject & aUbo,
                                const LightViewProjection & aLightViewProjection);

void updateOffsetInLightViewProjectionUbo(const graphics::UniformBufferObject & aUbo,
                                          const LightViewProjection & aLightViewProjection);


} // namespace ad::renderer