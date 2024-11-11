#include "Passes.h"
#include "ShadowCascadeBlock.h"
#include "math/Utilities.h"

#include "../ColorPalettes.h"
#include "../DebugDrawUtilities.h"
#include "../Logging.h"
// This is almost circular, but we need a lot of stuff, such as passes and HardcodedUbos
// This should vanish when we end-up with a cleaner design of the Graph / Passes abstractions.
#include "../TheGraph.h"

#include <limits>
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


namespace {

    void setupShadowMap(ShadowMapping & aShadowMapping, math::Size<2, GLsizei> aLayerSize, GLsizei aLayers)
    {
        const graphics::Texture & shadowMap = *aShadowMapping.mShadowMap;

        graphics::ScopedBind boundTexture{shadowMap};

        // We want to resize when switching between CSM / Single
        // which is not convenient with immutable storage
    //#define IMMUTABLE_SHADOWMAP
    #if defined(IMMUTABLE_SHADOWMAP)
        gl.TexStorage3D(shadowMap.mTarget,
                        1,
                        GL_DEPTH_COMPONENT24,
                        aLayerSize.width(),
                        aLayerSize.height(),
                        aLayers);
        assert(graphics::isImmutableFormat(shadowMap));
    #else
        gl.TexImage3D(shadowMap.mTarget, 
                    0,
                    GL_DEPTH_COMPONENT24,
                    aLayerSize.width(),
                    aLayerSize.height(),
                    aLayers,
                    0,
                    GL_DEPTH_COMPONENT,
                    GL_FLOAT,
                    nullptr);
    #endif //IMMUTABLE_SHADOWMAP

        aShadowMapping.mMapSize = aLayerSize;

        // Note: we would like to check for texture completeness (in the context of sampling it)
        // but attaching it to a framebuffer and checking framebuffer completeness does not
        // help with this scenario (I suppose this approach checks for "write" completeness).
        //assert(isTextureComplete(shadowMap));

        // Set texture comparison mode, allowing to compare the depth component to a reference value
        glTexParameteri(shadowMap.mTarget, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(shadowMap.mTarget, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        glTexParameteri(shadowMap.mTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(shadowMap.mTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(shadowMap.mTarget, 
                        GL_TEXTURE_BORDER_COLOR,
                        math::hdr::Rgba_f{1.f, 0.f, 0.f, 0.f}.data());

        // We disable mipmap minification filter, otherwise a mutable texture
        // would not be mipmap complete, and could not be sampled.
        // (will be touched again by the PCF parameter)
        glTexParameteri(shadowMap.mTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

} // unnamed namespace


void ShadowMapping::prepareShadowMap()
{
    static_assert(gCascadesPerShadow == 4);
    // To be fair when comparing methods, the non-cascaded variant should use 4 times bigger individual shadow maps.
    if (mControls.mUseCascades)
    {
        setupShadowMap(*this, ShadowMapping::gTexelPerLight / 2, gMaxShadowMaps);
    }
    else
    {
        setupShadowMap(*this, ShadowMapping::gTexelPerLight, gMaxShadowLights);
    }
    mIsSetupForCsm = mControls.mUseCascades;

    {
        graphics::ScopedBind boundTexture{*mShadowMap};
        // GL_LINEAR enables hardware PCF
        const GLenum filter = (mControls.mUseHardwarePcf ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(mShadowMap->mTarget, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(mShadowMap->mTarget, GL_TEXTURE_MAG_FILTER, filter);
    }
    mIsSetupForPcf = mControls.mUseHardwarePcf;
}


void ShadowMapping::reviewControls()
{
    // Note: This is wastefull, doing the whole preparation (including immage specification)
    // each time any checked value is changed.
    // Yet this is not intended to be runtime dynamic settings in production.
    if(   mIsSetupForCsm != mControls.mUseCascades
       || mIsSetupForPcf != mControls.mUseHardwarePcf)
    {
        prepareShadowMap();
    }
}


/// @brief From a frustum defined by an inverse projection matrix, return the tight rectangle
/// bounding the X-Y plane of the frustum.
math::Rectangle<GLfloat> getFrustumSideBounds(math::Matrix<4, 4, GLfloat> aFrustumClipToLightSpace)
{
    using math::homogeneous::homogenize;

    math::Position<2, GLfloat> low = homogenize(gNdcUnitCorners[0] * aFrustumClipToLightSpace).xy();
    math::Position<2, GLfloat> high = low;

    for(std::size_t idx = 1; idx != gNdcUnitCorners.size(); ++idx)
    {
        math::Position<2, GLfloat> cornerInLightSpace = homogenize(gNdcUnitCorners[idx] * aFrustumClipToLightSpace).xy();
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


/// @brief Clips triangles formed by the `aAabb` against the X and Y planes defined by `aClippingFrustymXYBorders`.
/// @return The minimum and maximum Z values of clipped triangles.
/// @param aAabb The axis aligned box to be clipped, in a space where the scene AABB is tight (usually, world-space).
/// Note: providing it in light space would not be productive, since an AABB in light space is already parallel to the light frustum.
/// @param aBoxToLight Transformation matrix from the space in which box is defined to light space.
/// @param aClippingFrustumXYBorders The X and Y borders of the light frustum (sometime called the shadow rectangle),
/// against which aAabb will be clipped.
std::pair<GLfloat /*Z-min*/, GLfloat /* Z-max */> tightenNearFar(
    math::Box<GLfloat> aAabb,
    math::LinearMatrix<3, 3, GLfloat> aBoxToLight,
    math::Rectangle<GLfloat> aClippingFrustumXYBorders,
    const ShadowMapping::Controls & c,
    math::hdr::Rgb_f aDebugDrawClippedTriangleColor)
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
        .mPosition  = {aClippingFrustumXYBorders.mPosition,  0.f},
        .mDimension = {aClippingFrustumXYBorders.mDimension, 0.f},
    };

    GLfloat zMin{std::numeric_limits<GLfloat>::max()};
    GLfloat zMax{std::numeric_limits<GLfloat>::lowest()};
    std::vector<Triangle> clipped;
    auto trackNearFar = [&zMin, &zMax, &clipped](const Triangle & aClippedTriangle)
    {
        zMin = std::min(zMin, minForComponent(2, aClippedTriangle));
        zMax  = std::max(zMax, maxForComponent(2, aClippedTriangle));
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

    if(c.mDebugDrawClippedTriangles)
    {
        const math::LinearMatrix<3, 3, GLfloat> lightToBox = aBoxToLight.inverse();
        for (const auto & tri : clipped)
        {
            debugDrawTriangle(Triangle{
                    math::homogeneous::makePosition(tri[0].xyz() * lightToBox),
                    math::homogeneous::makePosition(tri[1].xyz() * lightToBox),
                    math::homogeneous::makePosition(tri[2].xyz() * lightToBox),
                },
                aDebugDrawClippedTriangleColor);
        }
    }

    return {zMin, zMax};
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


struct DebugColors
{
    math::hdr::Rgb_f mViewFrustrum;
    math::hdr::Rgb_f mLight;
};


/// @brief Compute a tight projection matrix, allowing to render a shadow map 
/// covering the part of a scene that is viewed by a camera.
/// @param aCameraFrustum The view-projection matrix of the camera, defining its frustum.
/// @return A projection matrix, to be composed with a world-to-light matrix by the client.
math::Matrix<4, 4, float> computeLightProjection(math::LinearMatrix<3, 3, GLfloat> aOrientationWorldToLight,
                                                 math::Box<GLfloat> aSceneAabb,
                                                 math::Matrix<4, 4, GLfloat> aCameraViewProjection,
                                                 const ShadowMapping::Controls & c,
                                                 math::Size<2, GLsizei> aMapSize,
                                                 DebugColors aDebugDrawColors)
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
    const math::Box<GLfloat> sceneAabb_light = aSceneAabb * aOrientationWorldToLight;
        //getBoxSideBounds(aSceneAabb, math::AffineMatrix<4, GLfloat>{aOrientationWorldToLight});
    const math::Rectangle<GLfloat> sceneAabbSide_light = sceneAabb_light.frontRectangle();

    math::Rectangle<GLfloat> effectiveRectangle_light = 
        c.mTightenLightFrustumXYToScene ?
            intersect(cameraFrustumSides_light, sceneAabbSide_light)
            : cameraFrustumSides_light;


    if(c.mMoveFrustumTexelIncrement)
    {
        // see: https://learn.microsoft.com/en-us/windows/win32/dxtecharts/common-techniques-to-improve-shadow-depth-maps?redirectedfrom=MSDN#moving-the-light-in-texel-sized-increments
        // see: https://github.com/walbourn/directx-sdk-samples/blob/a5a073eb857d5af5bb5c35b9340b307a0c9093a6/CascadedShadowMaps11/CascadedShadowsManager.cpp#L932-L954
        math::Rectangle<GLfloat> & r = effectiveRectangle_light;
        math::Size<2, GLfloat> worldUnitsPerTexel = 
            r.dimension().cwDiv(aMapSize.as<math::Size, GLfloat>());

        r.origin() = floor(r.origin().cwDiv(worldUnitsPerTexel.as<math::Position>()));
        r.origin().cwMulAssign(worldUnitsPerTexel.as<math::Position>());

        r.dimension() = floor(r.dimension().cwDiv(worldUnitsPerTexel));
        r.dimension().cwMulAssign(worldUnitsPerTexel);
    }

    // Draw the shadow rectangle: rectangle bounding the light frustum in X and Y.
    // Note: We could be finer grained, but at the moment we draw the shadow rectangle when we draw the frusta.
    if(c.mDebugDrawShadowPlanes)
    {
        using Level = ::ad::snac::DebugDrawer::Level;

        // For a rotation matrix, the transpose is the inverse
        const math::LinearMatrix<3, 3, GLfloat> orientationLightToWorld = aOrientationWorldToLight.transpose();

        static auto drawPlane = [](math::LinearMatrix<3, 3, GLfloat> aLightToWorld,
                                   math::Rectangle<GLfloat> aRectangle_light,
                                   Level aLevel,
                                   DebugColors aDebugDrawColors)
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
                {aDebugDrawColors.mViewFrustrum, 1.f},
                {aDebugDrawColors.mLight, 1.f}

            );
        };

        drawPlane(orientationLightToWorld, effectiveRectangle_light, Level::debug, aDebugDrawColors);
    }

    auto far = sceneAabb_light.zMin();
    auto depth = sceneAabb_light.depth();

    if(c.mTightenFrustumDepthToClippedScene)
    {
        // Note: Here, we have to provide the **world** AABB for the scene,
        // (whose corners will be transformed from world to light frame in the function).
        // Not the **light** AABB for the scene.
        // (The side of the light AABB are already parallel to any other light AABB, including the shadow frustum.)
        // Important: The world and light frames are right-handed: zMin is far-plane's position, zMax is near-plane's.
        GLfloat near;
        std::tie(far, near) = tightenNearFar(aSceneAabb, 
                                             aOrientationWorldToLight,
                                             effectiveRectangle_light,
                                             c,
                                             aDebugDrawColors.mViewFrustrum);
        depth = near - far;

        // TODO turn the inverse condition into simple assertions
        if(far < sceneAabb_light.zMin() && !math::absoluteTolerance(far, sceneAabb_light.zMin(), 1.E-5f))
        {
            SELOG(warn)("Whole scene shadow far plane '{}' was tighter than clipped-scene far plane '{}'.", 
                        sceneAabb_light.zMin(),
                        far);
        }
        if(near > sceneAabb_light.zMax() && !math::absoluteTolerance(near, sceneAabb_light.zMax(), 1.E-5f))
        {
            SELOG(warn)("Whole scene near far  plane '{}' was tighter than clipped-scene near plane '{}'.", 
                        sceneAabb_light.zMax(),
                        near);
        }
    }

    math::Position<3, GLfloat> frustumBoxPosition{effectiveRectangle_light.mPosition, far};
    math::Size<3, GLfloat> frustumBoxDimension{effectiveRectangle_light.mDimension, depth};
    return graphics::makeOrthographicProjection({frustumBoxPosition, frustumBoxDimension});
}


/// @brief Z-partitioning of the camera frustum.
/// @return An array of view-projection matrices, one per camera-subfrustum.
std::array<math::Matrix<4, 4, GLfloat>, gCascadesPerShadow> zParitionCameraFrustum(const Camera & aCamera,
                                                                                      ShadowCascadeBlock & aShadowCascadeBlock,
                                                                                      const ShadowMapping::Controls & c)
{
    const auto [cameraNear, cameraFar] = getNearFarPlanes(aCamera);
    // Minimum of negative values will return the value with largest absolute value.
    const float clampedCameraNear = std::min(cameraNear, c.mCsmNearPlaneLimit);
    // Computation of Z-partitioning ratio
    const float distanceRatio = std::pow(cameraFar/clampedCameraNear, 1.f/gCascadesPerShadow);

    std::array<math::Matrix<4, 4, GLfloat>, gCascadesPerShadow> frustaCascade;
    // Warning: #cascade_hardcode_4 we are hardcoding the fact there are exactly 4 cascades here.
    static_assert(gCascadesPerShadow == 4);

    // For each cascade, calculate the view-frustum bounds
    // We follow fit-to-cascade, where cascade N-1 far-plane becomes cascade N near-plane.
    // see: https://learn.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps#fit-to-cascade
    float near = cameraNear; 
    // Note: we treat the first subfrusta far plane computation separately
    // (it does not use the actual frusta near, but a clamped near value)
    float far = clampedCameraNear * distanceRatio;
    for(unsigned int cascadeIdx = 0; cascadeIdx != gCascadesPerShadow; ++cascadeIdx)
    {
        frustaCascade[cascadeIdx] = aCamera.getParentToCamera() * getProjectionChangeNearFar(aCamera, near, far);

        if(c.mDebugDrawFrusta) 
        {
            debugDrawViewFrustum(
                frustaCascade[cascadeIdx].inverse(),
                drawer::gShadow,
                hdr::gColorBrewerSet1_linear[cascadeIdx]);
        }
        aShadowCascadeBlock.mCascadeFarPlaneDepths_view[cascadeIdx] = far;
        near = far;
        far = near * distanceRatio;
    }
    // The last far value, which was assigned to near, should match the overall camera far
    assert(math::absoluteTolerance(near, cameraFar, 100 * std::numeric_limits<float>::epsilon()));

    return frustaCascade;
}


LightsDataInternal fillShadowMap(const ShadowMapping & aPass, 
                                 const RepositoryTexture & aTextureRepository,
                                 Storage & aStorage,
                                 const GraphShared & aGraphShared,
                                 const ViewerPartList & aPartList,
                                 math::Box<GLfloat> aSceneAabb_world,
                                 const Camera & aCamera,
                                 const LightsDataUi & aLights)
{
    const ShadowMapping::Controls & c = aPass.mControls;
    const DepthMethod method = c.mUseCascades ? DepthMethod::Cascaded : DepthMethod::Single;
    Handle<graphics::Texture> shadowMap = aPass.mShadowMap;

    // Z-partitioning of the camera frustum (only usefull for CSM)
    std::array<math::Matrix<4, 4, GLfloat>, gCascadesPerShadow> frustaCascade;
    if (method == DepthMethod::Cascaded)
    {
        ShadowCascadeBlock shadowCascadeBlock;
        frustaCascade = zParitionCameraFrustum(aCamera, shadowCascadeBlock, c);

        shadowCascadeBlock.mDebugTintCascade = c.mTintViewByCascadeIdx;
        // Note: In production, it is likely that a given camera has constant Z-partitioning, so the shadow cascade UBO
        // could be loaded once.
        // Note: this load is usefull for subsequent scene renderings, not to render the shadow map in itself.
        // It is a bit of a smell to have side effects for subsequent passes "hidden" in this specific function
        // (it does not fell well-structured).
        // The goal of the frame graph is to make this kind of data dependency very explicit, such as:
        // This call produces the ShadowCascadeUbo, which is later consummed by other passes (e.g. forward pass)
        proto::loadSingle(*aGraphShared.mUbos.mShadowCascadeUbo, shadowCascadeBlock, graphics::BufferHint::DynamicDraw);
    }

    // Will receive the view-projection matrix (ie. form world to light clipping space)
    // for each light projecting a shadow.
    LightViewProjection lightViewProjection;

    // Enable slope-scale bias
    auto scopePolygonOffset = graphics::scopeFeature(GL_POLYGON_OFFSET_FILL, true);
    glPolygonOffset(c.mSlopeScale, c.mUnitScale);

    static constexpr GLfloat clearDepth = 1.f;
    glClearTexImage(*shadowMap, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &clearDepth);

    // Will remain the bound FBO while all the shadow maps are rendered
    graphics::ScopedBind boundFbo{aPass.mFbo, graphics::FrameBufferTarget::Draw};

    // The viewport size is be the same for each shadow map.
    glViewport(0, 0, aPass.mMapSize.width(), aPass.mMapSize.height());

    std::span<const DirectionalLight> aDirectionalLights = aLights.spanDirectionalLights();
    LightsDataInternal result;
    // Keep the count of lights projecting a shadow
    GLuint shadowLightIdx = 0;
    const GLuint cascadesPerLight = (method == DepthMethod::Cascaded ? gCascadesPerShadow : 1);

    // Compute the lights view-projection matrices (times the number of cascades when using CSM)
    for(GLuint directionalIdx = 0;
        directionalIdx != aDirectionalLights.size();
        ++directionalIdx)
    {
        if (aLights.mDirectionalLightProjectShadow[directionalIdx])
        {
            assert(shadowLightIdx < gMaxShadowLights);
            // To avoid the multiplication in the fragment shader, we direclty store the base cascade index for current light
            result.mDirectionalLightShadowMapIndices[directionalIdx] = shadowLightIdx * cascadesPerLight;

            const DirectionalLight light = aDirectionalLights[directionalIdx];

            // This is a pure orientation: a directional light has no "position", we can set its origin to the other space origin.
            const math::LinearMatrix<3, 3, GLfloat> orientationWorldToLight = alignMinusZ(light.mDirection);

            if(method == DepthMethod::Cascaded)
            {
                for(unsigned int cascadeIdx = 0; cascadeIdx != gCascadesPerShadow; ++cascadeIdx)
                {
                    math::Matrix<4, 4, float> cascadeProjection = computeLightProjection(orientationWorldToLight,
                                                                                         aSceneAabb_world,
                                                                                         frustaCascade[cascadeIdx],
                                                                                         c,
                                                                                         aPass.mMapSize,
                                                                                         {
                                                                                             .mViewFrustrum = hdr::gColorBrewerSet1_linear[cascadeIdx],
                                                                                             .mLight = light.mDiffuseColor * 0.5f
                                                                                         });

                    // Add the light view-projection to the collection
                    lightViewProjection.mLightViewProjections[shadowLightIdx * gCascadesPerShadow + cascadeIdx] =
                        math::AffineMatrix<4, GLfloat>{orientationWorldToLight}
                        * cascadeProjection;
                    lightViewProjection.mLightViewProjectionCount++;

                    // At the moment, draw all if the requested idx above max.
                    if(c.mDebugDrawWhichCascade == (int)cascadeIdx || c.mDebugDrawWhichCascade >= (int)gCascadesPerShadow) 
                    {
                        debugDrawViewFrustum(lightViewProjection.mLightViewProjections[shadowLightIdx * gCascadesPerShadow + cascadeIdx].inverse(),
                                            drawer::gLight,
                                            light.mDiffuseColor);
                    }
                }

            }
            else
            {
                math::Matrix<4, 4, float> fullProjection = computeLightProjection(orientationWorldToLight,
                                                                                  aSceneAabb_world,
                                                                                  aCamera.assembleViewProjection(),
                                                                                  c,
                                                                                  aPass.mMapSize,
                                                                                  {
                                                                                          .mViewFrustrum = hdr::gCameraFrustumColor,
                                                                                          .mLight = light.mDiffuseColor * 0.5f
                                                                                  });

                // Add the light view-projection to the collection
                lightViewProjection.mLightViewProjections[shadowLightIdx] =
                    math::AffineMatrix<4, GLfloat>{orientationWorldToLight}
                    * fullProjection;
                lightViewProjection.mLightViewProjectionCount++;

                if(c.mDebugDrawWhichCascade >= 0) 
                {
                    debugDrawViewFrustum(lightViewProjection.mLightViewProjections[shadowLightIdx].inverse(),
                                        drawer::gLight,
                                        light.mDiffuseColor);
                }
                //
                // Render shadow map (for single method)
                //

                // Attach a texture as the logical buffer of the FBO
                // This is not layered, as we attach a single layer from the shadow-maps array.
                // (The layer we intend to render to via `passOpaqueDepth()`).
                {
                    // The attachment is permanent, no need to recreate it each time the FBO is bound
                    gl.FramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, *shadowMap, /*mip map level*/0, shadowLightIdx);
                    assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
                }

                // TODO Ad 2024/10/11: #parameterize_shaders if we were able to select a program with the correct
                // number of GS invocations (lights) we would only need a single "draw call".
                // and it would be more uniform with the CSM version: using the same instanced-GS program,
                // done once after this for-loop.
                loadCameraUbo(*aGraphShared.mUbos.mViewingUbo, 
                            GpuViewProjectionBlock{orientationWorldToLight, fullProjection});
                passOpaqueDepth(aGraphShared, aPartList, aTextureRepository, aStorage, DepthMethod::Single);
            }

            ++shadowLightIdx;
        }
        else
        {
            // Actually the value is already written by initialization.
            result.mDirectionalLightShadowMapIndices[directionalIdx] = gNoEntryIndex;
        }
    }

    loadLightViewProjectionUbo(*aGraphShared.mUbos.mLightViewProjectionUbo, lightViewProjection);

    //
    // Render shadow maps
    //
    if(method == DepthMethod::Cascaded)
    {
        // Attach a texture as the logical buffer of the FBO
        // Since the texture is an array, the attachment is layered.
        {
            // TODO: The attachment is permanent, no need to recreate it each time the FBO is bound
            gl.FramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, *shadowMap, /*mip map level*/0);
            assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        }

        // TODO Ad 2024/10/11: #parameterize_shaders if we were able to select a program with the correct
        // number of GS invocations (lights * cascades) we would only need a single "draw call".
        for(GLuint directionalIdx = 0;
            directionalIdx != aDirectionalLights.size();
            ++directionalIdx)
        {
            lightViewProjection.mLightViewProjectionOffset = gCascadesPerShadow * directionalIdx;
            updateOffsetInLightViewProjectionUbo(*aGraphShared.mUbos.mLightViewProjectionUbo,
                                                lightViewProjection);
            // TODO avoid the instanciated stuff when the cascades are disabled
            passOpaqueDepth(aGraphShared, aPartList, aTextureRepository, aStorage, DepthMethod::Cascaded);
        }
    }
    else
    {
        // Done above, directly in the loop where we compute the lights' view-projection.
    }

    return result;
}



} // namespace ad::renderer
