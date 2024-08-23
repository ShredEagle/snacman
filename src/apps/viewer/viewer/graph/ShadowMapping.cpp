#include "ShadowMapping.h"

#include "../DebugDrawUtilities.h"
// This is almost circular, but we need a lot of stuff, such as passes and HardcodedUbos
// This should vanish when we end-up with a cleaner design of the Graph / Passes abstractions.
#include "../TheGraph.h"

#include <profiler/GlApi.h>

#include <snac-renderer-V2/Camera.h>


namespace ad::renderer {


LightViewProjection fillShadowMap(const ShadowMapping & aPass, 
                                  const RepositoryTexture & aTextureRepository,
                                  Storage & aStorage,
                                  const TheGraph & aGraph,
                                  const ViewerPartList & aPartList,
                                  Handle<const graphics::Texture> aShadowMap,
                                  std::span<const DirectionalLight> aDirectionalLights,
                                  bool aDebugDrawFrusta)
{
    const ShadowMapping::Controls & c = aPass.mControls;

    LightViewProjection lightViewProjection;

    auto scopePolygonOffset = graphics::scopeFeature(GL_POLYGON_OFFSET_FILL, true);
    glPolygonOffset(c.mSlopeScale, c.mUnitScale);

    static constexpr GLfloat clearDepth = 1.f;
    glClearTexImage(*aShadowMap, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &clearDepth);

    // Will remain the bound FBO while all the shadow maps are rendered
    graphics::ScopedBind boundFbo{aPass.mFbo, graphics::FrameBufferTarget::Draw};

    // The viewport size is be the same for each shadow map.
    glViewport(0, 0, ShadowMapping::gTextureSize.width(), ShadowMapping::gTextureSize.height());

    for(GLuint directionalIdx = 0;
        directionalIdx != aDirectionalLights.size();
        ++directionalIdx)
    {
        // TODO this should disappear when we have a better approach to defining which lights can cast shadow.
        // Atm, we hardcode that each directional light cast a shadow, thus allowing to index shadow with directionalIdx
        assert(directionalIdx < gMaxShadowLights);

        // Attach a texture as the logical buffer of the FBO
        {
            // The attachment is permanent, no need to recreate it each time the FBO is bound
            gl.FramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, *aShadowMap, /*mip map level*/0, directionalIdx);
            assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        }

        const DirectionalLight light = aDirectionalLights[directionalIdx];
        Camera lightViewpoint;
        // TODO handle lights for which the default up direction does not work (because it is the light gaze direction)
        lightViewpoint.setPose(graphics::getCameraTransform(
            -5 * light.mDirection.as<math::Position>(),
            light.mDirection
        ));
        // TODO handle the size of the orthographic projection
        lightViewpoint.setupOrthographicProjection({
            .mAspectRatio = 1.f,
            .mViewHeight = 6.f,
            .mNearZ = 0.f,
            .mFarZ = -10.f,
        });

        // TODO: The actual pass is somewhere down there:

        // Note: We probably only need the assembled view-projection matrix for lights 
        // (since we do not do any fragment computation in light space)
        // We could probably consolidate that with the "LightViewProjection" populated below
        loadCameraUbo(*aGraph.mUbos.mViewingUbo, lightViewpoint);

        aGraph.passOpaqueDepth(aPartList, aTextureRepository, aStorage);

        // Add the light view-projection to the collection, for main rendering
        lightViewProjection.mLightViewProjectionCount++;
        lightViewProjection.mLightViewProjections[directionalIdx] = lightViewpoint.assembleViewProjection();

        if(aDebugDrawFrusta) 
        {
            debugDrawViewFrustum(lightViewProjection.mLightViewProjections[directionalIdx].inverse(),
                                 drawer::gLight,
                                 light.mDiffuseColor);
        }
    }

    return lightViewProjection;
}



} // namespace ad::renderer