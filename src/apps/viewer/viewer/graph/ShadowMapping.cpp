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


// TODO Ad 2024/08/29: Consolidate with function above
math::Rectangle<GLfloat> getBoxSideBounds(math::Box<GLfloat> aBox, math::AffineMatrix<4, GLfloat> aBoxToLightSpace)
{
    using math::homogeneous::normalize;

    math::Position<2, GLfloat> low = normalize(math::homogeneous::makePosition(aBox.cornerAt(0)) * aBoxToLightSpace).xy();
    math::Position<2, GLfloat> high = low;

    for(std::size_t idx = 1; idx != aBox.gCornerCount; ++idx)
    {
        math::Position<2, GLfloat> cornerInLightSpace = normalize(math::homogeneous::makePosition(aBox.cornerAt(idx)) * aBoxToLightSpace).xy();
        low = min(low, cornerInLightSpace);
        high = max(high, cornerInLightSpace);
    }

    return math::Rectangle<GLfloat>{
        .mPosition = low,
        .mDimension = (high - low).as<math::Size>(),
    };
}


// TODO Ad 2024/08/29: Move to math
math::Rectangle<GLfloat> intersect(math::Rectangle<GLfloat> aLhs, const math::Rectangle<GLfloat> & aRhs)
{
    const math::Position<2, GLfloat> topRight = math::min(aLhs.topRight(), aRhs.topRight());
    aLhs.mPosition = math::max(aLhs.mPosition, aRhs.mPosition);
    aLhs.mDimension = (topRight - aLhs.mPosition).as<math::Size>();

    if(aLhs.mDimension.width() <= 0.f || aLhs.mDimension.height() <= 0.f)
    {
        aLhs.mDimension = {0.f, 0.f};
    }
    return aLhs;
}


/// @brief Construct a rotation matrix from canonical space to camera space.
/// @param aGazeDirection The camera view direction, expressed in canonical space.
///
/// The rotation matrix transforms coordinates from the basis in which
/// `aGazeDirection`  is defined, to a basis whose -Z is `aGazeDirection`.
/// (Alternatively, this matrix actively rotates aGazeDirection onto -Z.)
math::LinearMatrix<3, 3, GLfloat> alignMinusZ(math::Vec<3, GLfloat> aGazeDirection)
{
    
    math::Vec<3, GLfloat> up{0.f, 1.f, 0.f};
    if(std::abs(aGazeDirection.dot(up)) > 0.9f)
    {
        up = {0.f, 0.f, -1.f};
    }
    auto cameraBase = 
        math::OrthonormalBase<3, GLfloat>::MakeFromWUp(-aGazeDirection, up);
    
    const auto & u = cameraBase.u();
    const auto & v = cameraBase.v();
    const auto & w = cameraBase.w();

    // TODO Ad 2024/08/29: This should be a math function
    return {
        u.x(),          v.x(),          w.x(),
        u.y(),          v.y(),          w.y(),
        u.z(),          v.z(),          w.z(),
    };
}

LightViewProjection fillShadowMap(const ShadowMapping & aPass, 
                                  const RepositoryTexture & aTextureRepository,
                                  Storage & aStorage,
                                  const TheGraph & aGraph,
                                  const ViewerPartList & aPartList,
                                  math::Box<GLfloat> aSceneAabb,
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

        //
        // Handle the orthographic projection.
        // The bounding box will determinate the size of the projection, and the position of the viewpoint.
        //
        
        // Compute the matrix transforming from the camera view clip space to the light space.
        // This is a pure orientation: a directional light has no "position", we can set its origin to world origin.
        const math::LinearMatrix<3, 3, GLfloat> orientationWorldToLight = alignMinusZ(light.mDirection);
        // The min/max of the camera view frustum in light space
        const math::Rectangle<GLfloat> cameraFrustumSides_light = 
            getFrustumSideBounds(aViewProjectionInverse * math::AffineMatrix<4, GLfloat>{orientationWorldToLight});

        const math::Rectangle<GLfloat> sceneAabbSide_light =
            getBoxSideBounds(aSceneAabb, math::AffineMatrix<4, GLfloat>{orientationWorldToLight});

        const math::Rectangle<GLfloat> effectiveRectangle_light = 
            c.mTightenLightFrustumToScene ?
                intersect(cameraFrustumSides_light, sceneAabbSide_light)
                : cameraFrustumSides_light;

        if(aDebugDrawFrusta) 
        {
            using Level = ::ad::snac::DebugDrawer::Level;

            // For a rotation matrix, the transpose is the inverse
            const math::LinearMatrix<3, 3, GLfloat> orientationLightToWorld = orientationWorldToLight.transpose();

            static auto drawPlane = [](math::LinearMatrix<3, 3, GLfloat> aLightToWorld,
                                       math::Rectangle<GLfloat> aRectangle_light,
                                       Level aLevel)
            {
                math::Position<3, GLfloat> center_world = 
                    math::Position<3, GLfloat>{aRectangle_light.center(), 0.f} 
                    * aLightToWorld;

                DBGDRAW(drawer::gLight, aLevel).addPlane(
                    center_world,
                    math::Vec<3, float>{1.f, 0.f, 0.f} * aLightToWorld, 
                    math::Vec<3, float>{0.f, 1.f, 0.f} * aLightToWorld,
                    5, 5, // subdivisions count
                    aRectangle_light.width(), aRectangle_light.height() // assumes no scaling in light to world
                );
            };

            drawPlane(orientationLightToWorld, cameraFrustumSides_light, Level::debug);
            drawPlane(orientationLightToWorld, sceneAabbSide_light, Level::debug);
            drawPlane(orientationLightToWorld, effectiveRectangle_light, Level::info);
        }

        math::Position<3, GLfloat> frustumBoxPosition{effectiveRectangle_light.mPosition, -10.f};
        math::Size<3, GLfloat> frustumBoxDimension{effectiveRectangle_light.mDimension, 20.f};
        // We assemble a view projection block, with camera transformation being just the rotation
        // and the orthographic view frustum set to the tight box computed abovd
        GpuViewProjectionBlock viewProjectionBlock{
            orientationWorldToLight,
            graphics::makeOrthographicProjection({frustumBoxPosition, frustumBoxDimension})};

        loadCameraUbo(*aGraph.mUbos.mViewingUbo, viewProjectionBlock);

        // TODO: The actual pass is somewhere down there:

        aGraph.passOpaqueDepth(aPartList, aTextureRepository, aStorage);

        // Add the light view-projection to the collection, for main rendering
        lightViewProjection.mLightViewProjectionCount++;
        lightViewProjection.mLightViewProjections[directionalIdx] = viewProjectionBlock.mViewingProjection;

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