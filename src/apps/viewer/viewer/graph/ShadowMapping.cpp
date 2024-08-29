#include "ShadowMapping.h"

#include "../DebugDrawUtilities.h"
// This is almost circular, but we need a lot of stuff, such as passes and HardcodedUbos
// This should vanish when we end-up with a cleaner design of the Graph / Passes abstractions.
#include "../TheGraph.h"

#include <profiler/GlApi.h>

#include <snac-renderer-V2/Camera.h>


namespace ad::renderer {


math::Rectangle<GLfloat> getFrustumSideBounds(math::Matrix<4, 4, GLfloat> aFrustumClipToLightSpace)
{
    using math::homogeneous::normalize;

    math::Position<2, GLfloat> low = normalize(gNdcUnitCorners[0] * aFrustumClipToLightSpace).xy();
    math::Position<2, GLfloat> high = low;

    for(std::size_t idx = 1; idx != gNdcUnitCorners.size(); ++idx)
    {
        math::Position<2, GLfloat> cornerInLightSpace = normalize(gNdcUnitCorners[idx] * aFrustumClipToLightSpace).xy();
        low = min(low, cornerInLightSpace);
        high = max(high, cornerInLightSpace);
    }

    return math::Rectangle<GLfloat>{
        .mPosition = low,
        .mDimension = (high - low).as<math::Size>(),
    };
}


LightViewProjection fillShadowMap(const ShadowMapping & aPass, 
                                  const RepositoryTexture & aTextureRepository,
                                  Storage & aStorage,
                                  const TheGraph & aGraph,
                                  const ViewerPartList & aPartList,
                                  math::Matrix<4, 4, GLfloat> aViewProjectionInverse,
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

        //
        // Handle the orthographic projection.
        // The bounding box will determinate the size of the projection, and the position of the viewpoint.
        //
        
        // Compute the matrix transforming from the camera view clip space to the light space.
        const math::Rectangle<GLfloat> lightFrustumSides = getFrustumSideBounds(aViewProjectionInverse * lightViewpoint.getParentToCamera());

        if(aDebugDrawFrusta) 
        {
            DBGDRAW_INFO(drawer::gLight).addPlane(
                  {0.f, 0.f, 0.f},
                  (math::Vec<4, float>{1.f, 0.f, 0.f, 0.f} * lightViewpoint.getParentToCamera().inverse()).xyz(), 
                  (math::Vec<4, float>{0.f, 1.f, 0.f, 0.f} * lightViewpoint.getParentToCamera().inverse()).xyz(),
                  5, 5,
                  lightFrustumSides.width(), lightFrustumSides.height());
        }

        lightViewpoint.setupOrthographicProjection({
            .mAspectRatio = math::getRatio<GLfloat>(lightFrustumSides.mDimension),
            .mViewHeight = lightFrustumSides.mDimension.height(),
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