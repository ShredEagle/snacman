#pragma once

#include <graphics/ApplicationGlfw.h>
#include <graphics/AppInterface.h>
#include <imguiui/ImguiUi.h>

#include <resource/ResourceFinder.h>

#include <snac-renderer/Camera.h>
#include <snac-renderer/DebugRenderer.h>
#include <snac-renderer/Render.h>
#include <snac-renderer/Rigging.h>


namespace ad {
namespace snac {


constexpr unsigned int gFontPixelHeight = 50;


enum class DebugSkeleton
{
    Hierarchy,
    Pose,
    Hybrid,
    None,
};


struct Scene
{
    struct Control
    {
        bool mShowBasis = true;
        DebugSkeleton mShownSkeleton = DebugSkeleton::None;
        std::unordered_map<std::string, NodeAnimation>::iterator mSelectedAnimation;
    };

    Scene(graphics::ApplicationGlfw & aGlfwApp,
          const filesystem::path & aGltfPath,
          DebugRenderer aDebugRenderer,
          const resource::ResourceFinder & aFinder);

    void update(double aDelta);

    void render(Renderer & aRenderer);

    int presentNodeTree(Node::Index aNode);

    graphics::AppInterface & mAppInterface;
    Camera mCamera;
    CameraBuffer mCameraBuffer;
    MouseOrbitalControl mCameraControl;
    const resource::ResourceFinder & mFinder;
    std::shared_ptr<graphics::AppInterface::SizeListener> mSizeListening;
    snac::DebugRenderer mDebugRenderer;
    imguiui::ImguiUi mImguiUi;

    Renderer mRenderer;
    InstanceStream mSingleInstance;
    Model mSkin;
    struct RiggingData
    {
        Rig mRig;
        std::unordered_map<std::string, NodeAnimation> mAnimations;
    } mRigging;
    graphics::UniformBufferObject mJointMatrices;
    double mAnimationTime = 0.;

    Control mControl;
};


} // namespace snac
} // namespace ad