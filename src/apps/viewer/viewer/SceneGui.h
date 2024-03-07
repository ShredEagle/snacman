#pragma once


#include <snac-renderer-V2/Model.h>


namespace ad::renderer {


struct Scene;


class SceneGui
{
    friend struct ViewerApplication;

public:
    SceneGui(Handle<Effect> aHighlight, const Storage & aStorage) :
        mHighlightMaterial{
            .mNameArrayOffset = 1, // hack to index the name at 0
            .mEffect = aHighlight},
        mStorage{aStorage}
    {}

    void present(Scene & aScene);

private:
    /// @return Hovered node
    Node * presentNodeTree(Node & aNode, unsigned int aIndex);
    void presentObject(const Object & aObject);
    void presentEffect(Handle<const Effect> aEffect);
    void presentShaders(const IntrospectProgram & aIntrospectProgram);
    void presentAnimations(Handle<AnimatedRig> mAnimatedRig);
    static void presentJointTree(const NodeTree<Rig::Pose> & aTree,
                                 NodeTree<Rig::Pose>::Node::Index aNodeIdx);

    void presentSelection();

    void showNodeWindow(Node & aNode);
    void showPartWindow(const Part & aPart);
    void showSourceWindow(const std::string & aSourceString);

    void handleHighlight(Node * aHovered);

    static const int gBaseFlags;
    static const int gLeafFlags;
    static const int gPartFlags;

    struct Options
    {
        bool mHighlightObjects = false;
    };
    
    Options mOptions;

    Handle<const Part> mSelectedPart = gNullHandle;
    // There are no handles on Nodes at the moment
    // The node Pose might be mutated, so it cannot be const
    Node * mSelectedNode = nullptr;
    Node * mHighlightedNode = nullptr;
    const std::string * mSelectedShaderSource = nullptr;

    struct AnimationSelection
    {
        // Note: Cannot be const, because we might animate the selected rig
        // (which mutates in place the Rig's JointTree).
        Handle</*const */AnimatedRig> mAnimatedRig = gNullHandle;
        const RigAnimation * mAnimation = nullptr;
    } mSelectedAnimation;

    Material mHighlightMaterial;

    const Storage & mStorage;
};


} // namespace ad::renderer