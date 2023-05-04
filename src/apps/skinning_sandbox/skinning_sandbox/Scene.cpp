#include "Scene.h"

#include "Logging.h"

#include <arte/gltf/Gltf.h>

#include <graphics/CameraUtilities.h>

#include <math/Transformations.h>

#include <renderer/BufferLoad.h>

#include <snac-renderer/DebugDrawer.h>
#include <snac-renderer/Instances.h>
#include <snac-renderer/ResourceLoad.h>

#include <snac-renderer/gltf/GltfLoad.h>
#include <snac-renderer/gltf/LoadBuffer.h>


namespace ad {
namespace snac {


const std::string gDebugDrawer = "skinning";

// Internal data structure used while loading the gltf animation
struct Anim
{
    using Index_t = arte::gltf::Index<arte::gltf::Accessor>;
    static constexpr Index_t gInvalidIndex = std::numeric_limits<Index_t::Value_t>::max();
    using Path = arte::gltf::animation::Target::Path;

    /// @return The index of the input (time) accessor.
    arte::gltf::Index<arte::gltf::Accessor>
    add(Path aPath, arte::Const_Owned<arte::gltf::animation::Sampler> aSampler)
    {
        // Only support linear interpolation at the moment.
        assert(aSampler->interpolation == arte::gltf::animation::Sampler::Interpolation::Linear);

        auto write = [](Index_t & aTarget, Index_t aValue)
        {
            if(aTarget != gInvalidIndex)
            {
                throw std::logic_error{"Duplicated node path in an animation."};
            }
            else
            {
                aTarget = aValue;
            }
        };

        switch(aPath)
        {
            case Path::Translation:
                write(translationAccessor, aSampler->output);
                break;
            case Path::Rotation:
                write(rotationAccessor, aSampler->output);
                break;
            case Path::Scale:
                write(scaleAccessor, aSampler->output);
                break;
            default:
                throw std::logic_error{"Unsupported animation target path."};
        }

        return aSampler->input;
    }

    bool isComplete() const
    {
        return translationAccessor != gInvalidIndex
            && rotationAccessor != gInvalidIndex
            && scaleAccessor != gInvalidIndex;
    }

    Index_t translationAccessor = gInvalidIndex;
    Index_t rotationAccessor = gInvalidIndex;
    Index_t scaleAccessor = gInvalidIndex;
};


void assertAccessor(arte::Const_Owned<arte::gltf::Accessor> aAccessor, 
                    arte::gltf::Accessor::ElementType aElementType,
                    arte::gltf::EnumType aComponentType) 
{
    assert(aAccessor->type == aElementType);
    assert(aAccessor->componentType == aComponentType);
}


NodeAnimation readAnimation(arte::Const_Owned<arte::gltf::Animation> aGltfAnimation,
                            const std::vector<Node::Index> & aGltfToTreeIndex)
{
    std::unordered_map<std::size_t/*nodetree index*/, Anim> perNode;

    Anim::Index_t inputAccessorIdx = Anim::gInvalidIndex;

    auto samplers = aGltfAnimation.iterate(&arte::gltf::Animation::samplers);
    for (const arte::gltf::animation::Channel & channel : aGltfAnimation->channels)
    {
        assert(channel.target.node.has_value());
        auto samplerInput =
            perNode[aGltfToTreeIndex[*channel.target.node]]
                .add(channel.target.path, samplers.at(channel.sampler));

        if(inputAccessorIdx != Anim::gInvalidIndex && inputAccessorIdx != samplerInput)
        {
            throw std::logic_error{"Expecting all channels of a given animation to have the same input."};
        }
        else
        {
            inputAccessorIdx = samplerInput;
        }
    }

    NodeAnimation animation;

    auto inputAccessor = aGltfAnimation.get(inputAccessorIdx);
    assertAccessor(inputAccessor, arte::gltf::Accessor::ElementType::Scalar, GL_FLOAT);
    animation.mTimepoints = loadTypedData<float>(inputAccessor);
    animation.mEndTime = std::get<arte::gltf::Accessor::MinMax<float>>(*inputAccessor->bounds).max[0];
    for(const auto & [nodeIndex, anim] : perNode)
    {
        if (!anim.isComplete())
        {
            throw std::logic_error{"Only supporting complete animations at the moment."};
        }

        auto translationAccessor = aGltfAnimation.get(anim.translationAccessor);
        assertAccessor(translationAccessor, arte::gltf::Accessor::ElementType::Vec3, GL_FLOAT);
        auto rotationAccessor = aGltfAnimation.get(anim.rotationAccessor);
        assertAccessor(rotationAccessor, arte::gltf::Accessor::ElementType::Vec4, GL_FLOAT);
        auto scaleAccessor = aGltfAnimation.get(anim.scaleAccessor);
        assertAccessor(scaleAccessor, arte::gltf::Accessor::ElementType::Vec3, GL_FLOAT);

        animation.mNodes.push_back(nodeIndex);
        animation.mKeyframesEachNode.push_back(NodeAnimation::NodeKeyframes{
            .mTranslations = loadTypedData<math::Vec<3, float>>(translationAccessor),
            .mRotations = loadTypedData<math::Quaternion<float>>(rotationAccessor),
            .mScales = loadTypedData<math::Vec<3, float>>(scaleAccessor),
        }); 
    }

    return animation;
}


std::unordered_map<std::string, NodeAnimation>
loadAnimations(const arte::Gltf & aGltf, const std::vector<Node::Index> & aGltfToTreeIndex)
{
    std::unordered_map<std::string, NodeAnimation> animations;

    for(arte::Const_Owned<arte::gltf::Animation> animation : aGltf.getAnimations())
    {
        std::string name = animation->name;
        name = (name.empty() ? "anim_#" + animation.id() : name);
        animations.emplace(name, readAnimation(animation, aGltfToTreeIndex));
    }

    return animations;
}


Rig loadSkin(const arte::Gltf & aGltf,
             const std::vector<Node::Index> & aGltfToTreeIndex,
             arte::gltf::Index<arte::gltf::Skin> aSkinIndex = 0)
{
    arte::Const_Owned<arte::gltf::Skin> skin = aGltf.get(aSkinIndex);

    Rig result{
        .mName = skin->name,
    };

    for (auto gltfNodeIdx : skin->joints)
    {
        result.mJoints.push_back(aGltfToTreeIndex[gltfNodeIdx]);
    }

    // Only support skin with inverse bind matrices
    assert(skin->inverseBindMatrices);
    auto ibmAccessor = skin.get(*skin->inverseBindMatrices);
    assertAccessor(ibmAccessor, arte::gltf::Accessor::ElementType::Mat4, GL_FLOAT);
    result.mInverseBindMatrices = loadTypedData<math::AffineMatrix<4, float>>(ibmAccessor);

    return result;
}


Scene::RiggingData loadRiggingData(const arte::Gltf & aGltf)
{
    Scene::RiggingData result;
    std::vector<Node::Index> indexMapping; // gltf to tree
    NodeTree<arte::gltf::Node::Matrix> scene;

    // TODO #anim should only load the joints instead of the whole scene.
    std::tie(scene, indexMapping) = loadHierarchy(aGltf, 0/*scene idx*/);

    result.mRig = loadSkin(aGltf, indexMapping, 0);
    result.mRig.mScene = std::move(scene);

    result.mAnimations = loadAnimations(aGltf, indexMapping);

    return result;
}


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
                    loadEffect(mFinder.pathFor("effects/MeshRigging.sefx"), TechniqueLoader{mFinder}))},
    mRigging{loadRiggingData(arte::Gltf{aGltfPath})}
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
    SELOG(info)("Found {} animations.", mRigging.mAnimations.size());
}


void Scene::update(double aDelta)
{
    auto & nodeTree = mRigging.mRig.mScene;
    const NodeAnimation & animation = mRigging.mAnimations.begin()->second;

    mAnimationTime += aDelta;

    //
    // Animation update
    //
    // TODO handle different playback modes (single, ping-pong, repeat)
    const auto sillyLValue = mRigging.mRig.computeJointMatrices();
    animation.animate((float)std::fmod(mAnimationTime, animation.mEndTime), nodeTree);
    graphics::load(mJointMatrices,
                   std::span{sillyLValue},
                   graphics::BufferHint::StreamDraw);

    DebugDrawer::StartFrame();
    mCamera.setPose(mCameraControl.getParentToLocal());
    mCameraBuffer.set(mCamera);

    DBGDRAW_INFO(gDebugDrawer).addBasis({});

    static std::array<math::hdr::Rgba_f, 3> gColorRotation{
        math::hdr::gMagenta<float>,
        math::hdr::gCyan<float>,
        math::hdr::gYellow<float>,
    };

    //for(Node::Index nodeIdx = 0; nodeIdx != nodeTree.mHierarchy.size(); ++nodeIdx)
    for(Node::Index nodeIdx : animation.mNodes)
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

    mImguiUi.newFrame();

    ImGui::Begin("Node hierarchy");
    for(Node::Index root = mRigging.mRig.mScene.mFirstRoot;
        root != Node::gInvalidIndex;
        root = mRigging.mRig.mScene.mHierarchy[root].mNextSibling)
    {
        presentNodeTree(root);
    }
    ImGui::End();

    Guard guiFrameScope{[this](){mImguiUi.render();}};
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
    auto & nodeTree = mRigging.mRig.mScene;

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