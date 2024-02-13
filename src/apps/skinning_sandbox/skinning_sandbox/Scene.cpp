#include "Scene.h"

#include "Logging.h"

#include <arte/gltf/Gltf.h>

#include <graphics/CameraUtilities.h>

#include <imguiui/Widgets.h>
#include <imguiui/Widgets-impl.h>

#include <math/Transformations.h>

#include <renderer/BufferLoad.h>

#include <snac-renderer-V1/DebugDrawer.h>
#include <snac-renderer-V1/Instances.h>
#include <snac-renderer-V1/ResourceLoad.h>

#include <snac-renderer-V1/gltf/GltfLoad.h>


namespace ad {
namespace snac {


const std::string gDebugDrawer = "skinning";


InstanceStream prepareSingleInstance()
{
    InstanceStream stream{initializeInstanceStream<PoseColor>()};
    std::array<PoseColor, 1> instance{
        PoseColor{
            .pose = math::Matrix<4, 4, float>::Identity(),
            .albedo = math::sdr::gWhite,
        },
    };
    stream.respecifyData(std::span{instance});
    return stream;
}


std::string to_string(DebugSkeleton aType)
{
    #define CASE(e) case DebugSkeleton::e: return #e;
    switch(aType)
    {
        CASE(Hierarchy)
        CASE(Pose)
        CASE(Hybrid)
        CASE(None)
        default:
            return "<INVALID>";
    }
    #undef CASE
}

Scene::Scene(graphics::ApplicationGlfw & aGlfwApp,
            const filesystem::path & aGltfPath,
             DebugRenderer aDebugRenderer,
             const resource::ResourceFinder & aFinder) :
    mAppInterface{*aGlfwApp.getAppInterface()},
    mCamera{math::getRatio<float>(mAppInterface.getFramebufferSize()), Camera::gDefaults},
    mCameraBuffer{mCamera},
    mCameraControl{mAppInterface.getWindowSize(), Camera::gDefaults.vFov},
    mFinder{aFinder},
    mDebugRenderer{std::move(aDebugRenderer)},
    mImguiUi{aGlfwApp},
    mSingleInstance{prepareSingleInstance()},
    // TOOD the gltf object is loaded twice, when loading the model and when loading the animation data
    mSkin{loadModel(aGltfPath,
                    loadEffect(mFinder.pathFor("effects/MeshRigging.sefx"), TechniqueLoader{mFinder}))}
{
    DebugDrawer::AddDrawer(gDebugDrawer);

    mAppInterface.registerCursorPositionCallback(
        [this](double xpos, double ypos)
        {
            if(!mImguiUi.isCapturingMouse())
            {
                mCameraControl.callbackCursorPosition(xpos, ypos);
            }
        });

    mAppInterface.registerMouseButtonCallback(
        [this](int button, int action, int mods, double xpos, double ypos)
        {
            if(!mImguiUi.isCapturingMouse())
            {
                mCameraControl.callbackMouseButton(button, action, mods, xpos, ypos);
            }
        });

    mAppInterface.registerScrollCallback(
        [this](double xoffset, double yoffset)
        {
            if(!mImguiUi.isCapturingMouse())
            {
                mCameraControl.callbackScroll(xoffset, yoffset);
            }
        });

    mSizeListening = mAppInterface.listenWindowResize(
        [this](const math::Size<2, int> & size)
        {
            mCamera.setPerspectiveProjection(math::getRatio<float>(size), Camera::gDefaults); 
            mCameraControl.setWindowSize(size);
        });

    //std::cerr << "Hierarchy:\n" << mRigging.mRig.mJoints << std::endl;
    SELOG(info)("Loaded model contains {} animations.", mSkin.mAnimations.size());
    assert(mSkin.mAnimations.size() > 0);
    mControl.mSelectedAnimation = mSkin.mAnimations.begin();
}


void Scene::update(double aDelta)
{
    mImguiUi.newFrame();
    Guard guiFrameScope{[this](){mImguiUi.render();}};

    ImGui::Begin("Controls");

    imguiui::addCombo("Animation",
                      mControl.mSelectedAnimation,
                      mSkin.mAnimations.begin(),
                      mSkin.mAnimations.end(),
                      [](std::unordered_map<std::string, NodeAnimation>::iterator aIt)
                      { return aIt->first; });
    const NodeAnimation & animation = mControl.mSelectedAnimation->second;
    auto & nodeTree = mSkin.mRig.mScene;

    mAnimationTime += aDelta;

    //
    // Animation update
    //

    // TODO handle different playback modes (single, ping-pong, repeat)
    animation.animate((float)std::fmod(mAnimationTime, animation.mEndTime), nodeTree);
    const auto sillyLValue = mSkin.mRig.computeJointMatrices();
    graphics::load(mJointMatrices,
                   std::span{sillyLValue},
                   graphics::BufferHint::StreamDraw);

    DebugDrawer::StartFrame();
    mCamera.setPose(mCameraControl.getParentToLocal());
    mCameraBuffer.set(mCamera);

    ImGui::Checkbox("Show canonical basis", &mControl.mShowBasis);
    if(mControl.mShowBasis)
    {
        DBGDRAW_INFO(gDebugDrawer).addBasis({});
    }

    static const auto skeletonTypes = {
        DebugSkeleton::None,
        DebugSkeleton::Hierarchy,
        DebugSkeleton::Pose,
        DebugSkeleton::Hybrid
    };
    imguiui::addCombo<DebugSkeleton>("Show skeleton", mControl.mShownSkeleton, std::span{skeletonTypes});

    static const std::array<math::hdr::Rgba_f, 3> gColorRotation{
        math::hdr::gMagenta<float>,
        math::hdr::gCyan<float>,
        math::hdr::gYellow<float>,
    };

    //for(Node::Index nodeIdx = 0; nodeIdx != nodeTree.mHierarchy.size(); ++nodeIdx)
    // TODO iterate over the rig instead of the animation
    //for(Node::Index nodeIdx : animation.mNodes)
    for(Node::Index nodeIdx : mSkin.mRig.mJoints)
    {
        // Draw a segment between each joint 
        // (sadly, this is missing "leaf bones", for joints without a child)
        if (mControl.mShownSkeleton == DebugSkeleton::Hierarchy
            || mControl.mShownSkeleton == DebugSkeleton::Hybrid)
        {
            if(Node::Index parentIdx = nodeTree.mHierarchy[nodeIdx].mParent; parentIdx != Node::gInvalidIndex)
            {
                // Only if the parent is also a joint do we trace the bone
                if(std::find(animation.mNodes.begin(), animation.mNodes.end(), parentIdx) != animation.mNodes.end())
                {
                    auto parentPos = math::Position<4, float>{0.f, 0.f, 0.f, 1.f} * nodeTree.mGlobalPose[parentIdx];
                    auto nodePos = math::Position<4, float>{0.f, 0.f, 0.f, 1.f} * nodeTree.mGlobalPose[nodeIdx];
                    DBGDRAW_INFO(gDebugDrawer).addLine(parentPos.xyz(),
                                                    nodePos.xyz(),
                                                    gColorRotation[nodeIdx % gColorRotation.size()]);

                }
            }
        }

        // Draw a 1 unit long segment along the Y axis of each joint local space.
        // Hybrid only draws it for joints with no child.
        if (mControl.mShownSkeleton == DebugSkeleton::Pose
                 || (mControl.mShownSkeleton == DebugSkeleton::Hybrid
                    && !nodeTree.hasChild(nodeIdx)) )
        {
            auto nodePos = math::Position<4, float>{0.f, 0.f, 0.f, 1.f} * nodeTree.mGlobalPose[nodeIdx];
            auto nodePosY = math::Position<4, float>{0.f, 1.f, 0.f, 1.f} * nodeTree.mGlobalPose[nodeIdx];
            DBGDRAW_INFO(gDebugDrawer).addLine(nodePos.xyz(),
                                                nodePosY.xyz(),
                                                math::hdr::gBlack<float>);
        }
    }

    ImGui::End();


    ImGui::Begin("Node hierarchy");
    for(Node::Index root = mSkin.mRig.mScene.mFirstRoot;
        root != Node::gInvalidIndex;
        root = mSkin.mRig.mScene.mHierarchy[root].mNextSibling)
    {
        presentNodeTree(root);
    }
    ImGui::End();
}


void Scene::render(Renderer & aRenderer)
{
    clear();

    math::Size<2, int> FbSize = mAppInterface.getFramebufferSize();

    auto viewportScope = graphics::scopeViewport({{0, 0}, FbSize});

    math::hdr::Rgb_f lightColor = to_hdr<float>(math::sdr::gWhite) * 0.8f;
    math::hdr::Rgb_f ambientColor = math::hdr::Rgb_f{0.1f, 0.1f, 0.1f};

    ProgramSetup setup{
        .mUniforms{
            {snac::Semantic::LightColor, {lightColor}},
            {snac::Semantic::LightPosition, {math::Vec<3, float>{0.f, 0.f, 0.f}}},
            {snac::Semantic::AmbientColor, {ambientColor}},
            {snac::Semantic::FramebufferResolution, FbSize},
            {snac::Semantic::ViewingMatrix, mCamera.assembleViewMatrix()},
        },
        .mUniformBlocks{
            {snac::BlockSemantic::Viewing, &mCameraBuffer.mViewing},
            {snac::BlockSemantic::JointMatrices, &mJointMatrices},
        },
    };

    //
    // Skin
    //
    {
        auto scopeDepth = graphics::scopeFeature(GL_DEPTH_TEST, true);
        static const Pass rigging{"Rigging"};
        for(const auto & model : mSkin.mParts)
        {
            rigging.draw(model, mSingleInstance, mRenderer, setup);
        }
    }

    //
    // Debug drawer
    //
    mDebugRenderer.render(DebugDrawer::EndFrame(), aRenderer, setup);

    //
    // DearImgui
    //
    mImguiUi.renderBackend();
}


int Scene::presentNodeTree(Node::Index aNode)
{
    auto & nodeTree = mSkin.mRig.mScene;

    int selected = -1;

    int flags = nodeTree.hasChild(aNode) ? 0 : ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
    const std::string  name = nodeTree.hasName(aNode) ? 
                               nodeTree.mNodeNames[aNode] : "Node_" + std::to_string(aNode);
    bool opened = ImGui::TreeNodeEx(&nodeTree.mHierarchy[aNode], flags, "%s", name.c_str());

    ImGui::PushID((int)aNode);

    if(ImGui::IsItemClicked(0))
    {
        selected = (int)aNode;
    }

    if(opened)
    {
        for(Node::Index child = nodeTree.mHierarchy[aNode].mFirstChild;
            child != Node::gInvalidIndex;
            child = nodeTree.mHierarchy[child].mNextSibling)
        {
            if(int childSelected = presentNodeTree(child);
               childSelected != -1)
            {
                selected = childSelected;
            }
        }
        ImGui::TreePop();
    }

    ImGui::PopID();

    return selected;
}


} // namespace snac
} // namespace ad