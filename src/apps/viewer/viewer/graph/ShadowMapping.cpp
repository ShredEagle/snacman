#include "ShadowMapping.h"

#include "Passes.h"
#include "ShadowCascadeBlock.h"

#include "../ColorPalettes.h"
#include "../DebugDrawUtilities.h"
#include "../Logging.h"
// This is almost circular, but we need a lot of stuff, such as passes and HardcodedUbos
// This should vanish when we end-up with a cleaner design of the Graph / Passes abstractions.
#include "../TheGraph.h"

#include <profiler/GlApi.h>

#include <snac-renderer-V2/Camera.h>
#include <snac-renderer-V2/RendererReimplement.h>


namespace ad::renderer {



void debugDrawTriangle(const Triangle & aTri, math::hdr::Rgb_f aColor = math::hdr::gGreen<GLfloat>)
{
    DBGDRAW_INFO(drawer::gShadow).addLine(
        aTri[0].xyz(), aTri[1].xyz(),
        aColor);
    DBGDRAW_INFO(drawer::gShadow).addLine(
        aTri[0].xyz(), aTri[2].xyz(),
        aColor);
    DBGDRAW_INFO(drawer::gShadow).addLine(
        aTri[1].xyz(), aTri[2].xyz(),
        aColor);
}


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
math::Box<GLfloat> getBoxSideBounds(math::Box<GLfloat> aBox, math::AffineMatrix<4, GLfloat> aBoxToLightSpace)
{
    using math::homogeneous::normalize;

    math::Position<3, GLfloat> low = 
        normalize(math::homogeneous::makePosition(aBox.cornerAt(0)) * aBoxToLightSpace).xyz();
    math::Position<3, GLfloat> high = low;

    for(std::size_t idx = 1; idx != aBox.gCornerCount; ++idx)
    {
        math::Position<3, GLfloat> cornerInLightSpace = 
            normalize(math::homogeneous::makePosition(aBox.cornerAt(idx)) * aBoxToLightSpace).xyz();
        low = min(low, cornerInLightSpace);
        high = max(high, cornerInLightSpace);
    }

    return math::Box<GLfloat>{
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


/// @param aAabb The axis aligned box to be clipped, in a space where the scene AABB is tight (usually, world-space).
/// Note: providing it in light space would not be productive, since an AABB in light space is already parallel to the light frustum.
/// @param aBoxToLight Transformation matrix from the space in which box is defined to light space.
/// @param aClippingFrustumXYBorders The X and Y borders of the light frustum (sometime called the shadow rectangle),
/// against which aAabb will be clipped.
std::pair<GLfloat /*near*/, GLfloat /* far */> tightenNearFar(
    math::Box<GLfloat> aAabb,
    math::LinearMatrix<3, 3, GLfloat> aBoxToLight,
    math::Rectangle<GLfloat> aClippingFrustumXYBorders,
    bool aDebugDrawClippedTriangles)
{
    // Indices of corners to tessellate a Box.
    static constexpr std::size_t gTriIdx[] = 
    {
        0,1,2,  1,2,3, // Z-min
        4,5,6,  5,6,7, // Z-max
        0,2,4,  2,4,6, // Left 
        1,3,5,  3,5,7, // Right
        0,1,4,  1,4,5, // Bottom
        2,3,6,  3,6,7  // Top
    };
    
    std::array<math::Position<4, GLfloat>, 8> corners_light;
    for(std::size_t cornerIdx = 0; cornerIdx != aAabb.gCornerCount; ++cornerIdx)
    {
        corners_light[cornerIdx] = 
            math::homogeneous::makePosition(aAabb.cornerAt(cornerIdx) * aBoxToLight);
    }

    // The clip interface requires a Box. But since we will not clip against the Z axis border,
    // a depth of zero is okay.
    const math::Box<GLfloat> clippingVolume{
        .mPosition  = {aClippingFrustumXYBorders.mPosition, 0.f},
        .mDimension = {aClippingFrustumXYBorders.mDimension, 0.f},
    };

    GLfloat near{std::numeric_limits<GLfloat>::max()};
    GLfloat far{std::numeric_limits<GLfloat>::min()};
    std::vector<Triangle> clipped;
    auto trackNearFar = [&near, &far, &clipped](const Triangle & aClippedTriangle)
    {
        near = std::min(near, minForComponent(2, aClippedTriangle));
        far  = std::max(far, maxForComponent(2, aClippedTriangle));
        clipped.push_back(aClippedTriangle);
    };

    for(std::size_t idx = 0; idx != std::size(gTriIdx); idx += 3)
    {
        std::array<math::Position<4, GLfloat>, 3> triangle{{
            corners_light[gTriIdx[idx]],
            corners_light[gTriIdx[idx + 1]],
            corners_light[gTriIdx[idx + 2]],
        }};

        clip(
            triangle,
            clippingVolume,
            trackNearFar,
            0,
            4 /* Only clip on l, r, b, t, not on near/far */);
    }

    if(aDebugDrawClippedTriangles)
    {
        const math::LinearMatrix<3, 3, GLfloat> lightToBox = aBoxToLight.inverse();
        for (const auto & tri : clipped)
        {
            debugDrawTriangle(Triangle{
                math::homogeneous::makePosition(tri[0].xyz() * lightToBox),
                math::homogeneous::makePosition(tri[1].xyz() * lightToBox),
                math::homogeneous::makePosition(tri[2].xyz() * lightToBox),
            });
        }
    }

    return {near, far};
}


/// @brief Return a projection matrix matching the current projection parameters of aCamera,
/// but with custom near & far planes.
math::Matrix<4, 4, float> getProjectionChangeNearFar(const Camera & aCamera, float aNearZ, float aFarZ)
{
    return std::visit(
        [aNearZ, aFarZ](auto params) -> math::Matrix<4, 4, float>
        {
            params.mNearZ = aNearZ;
            params.mFarZ = aFarZ;
            return graphics::makeProjection(params);
        },
        aCamera.getProjectionParameters()
    );
}


struct DebugPlaneColors
{
    math::hdr::Rgb_f mOutline;
    math::hdr::Rgb_f mSubdiv;
};


/// @brief Compute a tight view projection block, used to render the shadow map of a light
/// illuminating a scene that is viewed by a camera.
/// @param aCameraFrustum The view-projection matrix of the camera, defining its frustum.
math::Matrix<4, 4, float> computeLightViewProjection(math::LinearMatrix<3, 3, GLfloat> aOrientationWorldToLight,
                                                     math::Box<GLfloat> aSceneAabb,
                                                     math::Matrix<4, 4, GLfloat> aCameraViewProjection,
                                                     const ShadowMapping::Controls & c,
                                                     std::optional<DebugPlaneColors> aDebugDrawColors // nullopt to disable debug drawing
                                                     )
{
    //
    // Handle the orthographic projection.
    // The bounding box will determinate the size of the projection, and the position of the viewpoint.
    //
    
    math::Matrix<4, 4, GLfloat> viewProjectionInverse = aCameraViewProjection.inverse();

    // The min/max of the camera view frustum in light space
    const math::Rectangle<GLfloat> cameraFrustumSides_light = 
        getFrustumSideBounds(
            // The matrix transforming from the camera view clip space to the light space.
            viewProjectionInverse * math::AffineMatrix<4, GLfloat>{aOrientationWorldToLight});

    // TODO can we replace with the aabb generic transformation?
    const math::Box<GLfloat> sceneAabb_light =
        getBoxSideBounds(aSceneAabb, math::AffineMatrix<4, GLfloat>{aOrientationWorldToLight});
    const math::Rectangle<GLfloat> sceneAabbSide_light = sceneAabb_light.frontRectangle();

    const math::Rectangle<GLfloat> effectiveRectangle_light = 
        c.mTightenLightFrustumXYToScene ?
            intersect(cameraFrustumSides_light, sceneAabbSide_light)
            : cameraFrustumSides_light;

    if(aDebugDrawColors) 
    {
        using Level = ::ad::snac::DebugDrawer::Level;

        // For a rotation matrix, the transpose is the inverse
        const math::LinearMatrix<3, 3, GLfloat> orientationLightToWorld = aOrientationWorldToLight.transpose();

        static auto drawPlane = [&aDebugDrawColors](math::LinearMatrix<3, 3, GLfloat> aLightToWorld,
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
                3, 3, // subdivisions count
                aRectangle_light.width(), aRectangle_light.height(), // assumes no scaling in light to world
                {aDebugDrawColors->mOutline, 1.f},
                {aDebugDrawColors->mSubdiv, 1.f}

            );
        };

        drawPlane(orientationLightToWorld, cameraFrustumSides_light, Level::trace);
        drawPlane(orientationLightToWorld, sceneAabbSide_light, Level::trace);
        drawPlane(orientationLightToWorld, effectiveRectangle_light, Level::debug);
    }

    auto near = sceneAabb_light.zMin();
    auto depth = sceneAabb_light.depth();

    if(c.mTightenFrustumDepthToClippedScene)
    {
        // Note: Here, we have to provide the corners of the **world** AABB for the scene,
        // expressed in light space (transformation from world to light frame currently occurs in the function).
        // Not the **light** AABB for the scene.
        // (The side of the light AABB are already parallel to any other light AABB, including the shadow frustum.)
        GLfloat far;
        std::tie(near, far) = tightenNearFar(aSceneAabb, 
                                             aOrientationWorldToLight,
                                             effectiveRectangle_light,
                                             c.mDebugDrawClippedTriangles);
        depth = far - near;

        // TODO turn the inverse condition into simple assertions
        if(near < sceneAabb_light.zMin() && !math::absoluteTolerance(near, sceneAabb_light.zMin(), 1.E-5f))
        {
            SELOG(warn)("Whole scene shadow near plane '{}' was tighter than clipped-scene near plane '{}'.", 
                        sceneAabb_light.zMin(),
                        near);
        }
        if(far > sceneAabb_light.zMax() && !math::absoluteTolerance(far, sceneAabb_light.zMax(), 1.E-5f))
        {
            SELOG(warn)("Whole scene shadow far  plane '{}' was tighter than clipped-scene far  plane '{}'.", 
                        sceneAabb_light.zMax(),
                        far);
        }
    }

    math::Position<3, GLfloat> frustumBoxPosition{effectiveRectangle_light.mPosition, near};
    math::Size<3, GLfloat> frustumBoxDimension{effectiveRectangle_light.mDimension, depth};
    return graphics::makeOrthographicProjection({frustumBoxPosition, frustumBoxDimension});
}


LightViewProjection fillShadowMap(const ShadowMapping & aPass, 
                                  const RepositoryTexture & aTextureRepository,
                                  Storage & aStorage,
                                  const GraphShared & aGraphShared,
                                  const ViewerPartList & aPartList,
                                  math::Box<GLfloat> aSceneAabb,
                                  const Camera & aCamera,
                                  Handle<const graphics::Texture> aShadowMap,
                                  std::span<const DirectionalLight> aDirectionalLights,
                                  bool aDebugDrawFrusta)
{
    const ShadowMapping::Controls & c = aPass.mControls;

    const math::AffineMatrix<4, float> cameraToWorld = aCamera.getParentToCamera().inverse();
    const math::Vec<3, float> camRight_world = math::Vec<3, float>{1.f, 0.f, 0.f} * cameraToWorld.getLinear();
    const math::Vec<3, float> camUp_world = math::Vec<3, float>{0.f, 1.f, 0.f} * cameraToWorld.getLinear();

    const auto [cameraNear, cameraFar] = getNearFarPlanes(aCamera);
    // Minimum of negative values will return the value with largest absolute value.
    float clampedCameraNear = std::min(cameraNear, c.mCsmNearPlaneLimit);
    // Computation of Z-partitioning ratio
    const float distanceRatio = std::pow(cameraFar/clampedCameraNear, 1.f/gMaxCascadesPerShadow);

    std::array<math::Matrix<4, 4, GLfloat>, gMaxCascadesPerShadow> frustaCascade;
    // Warning: #cascade_hardcode_4 we are hardcoding the fact there are exactly 4 cascades here.
    static_assert(gMaxCascadesPerShadow == 4);
    ShadowCascadeBlock shadowCascadeBlock;

    // For each cascade, calculate the view-frustum bounds
    // We follow fit-to-cascade, where cascade N-1 far-plane becomes cascade N near-plane.
    // see: https://learn.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps#fit-to-cascade
    float near = cameraNear; 
    // Note: we treat the first subfrusta far plane computation separately
    // (it does not use the actual frusta near, but a clamped near value)
    float far = clampedCameraNear * distanceRatio;
    for(unsigned int cascadeIdx = 0; cascadeIdx != gMaxCascadesPerShadow; ++cascadeIdx)
    {
        frustaCascade[cascadeIdx] = aCamera.getParentToCamera() * getProjectionChangeNearFar(aCamera, near, far);

        if(aDebugDrawFrusta) 
        {
            debugDrawViewFrustum(
                frustaCascade[cascadeIdx].inverse(),
                drawer::gShadow,
                hdr::gColorBrewerSet1_linear[cascadeIdx]);
        }
        shadowCascadeBlock.mCascadeFarPlaneDepths_view[cascadeIdx] = far;
        near = far;
        far = near * distanceRatio;
    }
    // The last far value, which was assigned to near, should match the overall camera far
    assert(near = cameraFar);

    LightViewProjection lightViewProjection;

    auto scopePolygonOffset = graphics::scopeFeature(GL_POLYGON_OFFSET_FILL, true);
    glPolygonOffset(c.mSlopeScale, c.mUnitScale);

    static constexpr GLfloat clearDepth = 1.f;
    glClearTexImage(*aShadowMap, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &clearDepth);

    // Will remain the bound FBO while all the shadow maps are rendered
    graphics::ScopedBind boundFbo{aPass.mFbo, graphics::FrameBufferTarget::Draw};

    // Attach a texture as the logical buffer of the FBO
    {
        // TODO: The attachment is permanent, no need to recreate it each time the FBO is bound
        gl.FramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, *aShadowMap, /*mip map level*/0);
        assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }

    // The viewport size is be the same for each shadow map.
    glViewport(0, 0, ShadowMapping::gTextureSize.width(), ShadowMapping::gTextureSize.height());

    // Compute the lights view-projection matrices (times the number of cascades when using CSM)
    for(GLuint directionalIdx = 0;
        directionalIdx != aDirectionalLights.size();
        ++directionalIdx)
    {
        // TODO this should disappear when we have a better approach to defining which lights can cast shadow.
        // Atm, we hardcode that each directional light cast a shadow, thus allowing to index shadow with directionalIdx
        assert(directionalIdx < gMaxShadowLights);

        const DirectionalLight light = aDirectionalLights[directionalIdx];

        // This is a pure orientation: a directional light has no "position", we can set its origin to the other space origin.
        const math::LinearMatrix<3, 3, GLfloat> orientationWorldToLight = alignMinusZ(light.mDirection);

        for(unsigned int cascadeIdx = 0; cascadeIdx != gMaxCascadesPerShadow; ++cascadeIdx)
        {
            math::Matrix<4, 4, float> cascadeViewProjection = computeLightViewProjection(orientationWorldToLight,
                                                                                         aSceneAabb,
                                                                                         frustaCascade[cascadeIdx],
                                                                                         c,
                                                                                         aDebugDrawFrusta ?
                                                                                             std::optional<DebugPlaneColors>{{
                                                                                                .mOutline = hdr::gColorBrewerSet1_linear[cascadeIdx],
                                                                                                .mSubdiv = light.mDiffuseColor * 0.5f}}
                                                                                             : std::nullopt
                                                                                        );

            // Add the light view-projection to the collection
            lightViewProjection.mLightViewProjections[directionalIdx * gMaxCascadesPerShadow + cascadeIdx] =
                math::AffineMatrix<4, GLfloat>{orientationWorldToLight}
                * cascadeViewProjection;
            lightViewProjection.mLightViewProjectionCount++;
        }

        if(aDebugDrawFrusta && (c.mDebugDrawWhichCascade >= 0)) 
        {
            const std::size_t whichCascade = c.mDebugDrawClippedTriangles; // largest
            debugDrawViewFrustum(lightViewProjection.mLightViewProjections[directionalIdx * gMaxCascadesPerShadow + c.mDebugDrawWhichCascade].inverse(),
                                 drawer::gLight,
                                 light.mDiffuseColor);
        }
    }

    loadLightViewProjectionUbo(*aGraphShared.mUbos.mLightViewProjectionUbo, lightViewProjection);

    // Clear must appear after the Framebuffer setup!
    gl.Clear(GL_DEPTH_BUFFER_BIT);

    // TODO Ad 2024/10/11: #parameterize_shaders if we were able to select a program with the correct
    // number of GS invocations (#lights x #cascade) we would only need a single "draw call".
    for(GLuint directionalIdx = 0;
        directionalIdx != aDirectionalLights.size();
        ++directionalIdx)
    {
        lightViewProjection.mLightViewProjectionOffset = gMaxCascadesPerShadow * directionalIdx;
        updateOffsetInLightViewProjectionUbo(*aGraphShared.mUbos.mLightViewProjectionUbo,
                                             lightViewProjection);
        passOpaqueDepth(aGraphShared, aPartList, aTextureRepository, aStorage, DepthMethod::Cascaded);
    }

    shadowCascadeBlock.mDebugTintCascade = c.mTintViewByCascadeIdx;
    // Note: In production, it is likely that a given camera has constant Z-partitioning, so the shadow cascade UBO
    // could be loaded once.
    // loadShadowCascadeUbo()
    proto::loadSingle(*aGraphShared.mUbos.mShadowCascadeUbo, shadowCascadeBlock, graphics::BufferHint::DynamicDraw);

    return lightViewProjection;
}



} // namespace ad::renderer