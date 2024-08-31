#pragma once


#include "Timing.h"

#include <snac-renderer-V2/Model.h>


namespace ad::renderer {


struct LightsData;
struct Scene;


class SceneGui
{
public:
    struct Options
    {
        bool mHighlightObjects = false;
        // Not sure it should be located here: the scene gui will offer the checkbox,
        // but it is unlikely to handle the actual rendering of light positions.
        bool mShowPointLights = true;
        bool mAreDirectionalLightsCameraSpace = false;
    };
    
    SceneGui(Handle<Effect> aHighlight, const Storage & aStorage) :
        mHighlightMaterial{
            .mNameArrayOffset = 1, // hack to index the name at 0
            .mEffect = aHighlight},
        mStorage{aStorage}
    {}

    void presentSection(Scene & aScene, const Timing & aTime);

    const Options & getOptions() const
    {
        return mOptions;
    }

    const Node * getHighlighted() const
    { return mHighlightedNode; }

private:
    /// @return Hovered node
    Node * presentNodeTree(Node & aNode, unsigned int aIndex, const Timing & aTime);
    void presentObject(const Object & aObject, Instance & aInstance, const Timing & aTime);
    void presentEffect(Handle<const Effect> aEffect);
    void presentLights(LightsData & aLightsData);
    void presentShaders(const IntrospectProgram & aIntrospectProgram);
    void presentAnimations(Handle<AnimatedRig> mAnimatedRig, Instance & aInstance, const Timing & aTime);
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

    Options mOptions;

    Handle<const Part> mSelectedPart = gNullHandle;
    // There are no handles on Nodes at the moment
    // The node Pose might be mutated, so it cannot be const
    Node * mSelectedNode = nullptr;
    Node * mHighlightedNode = nullptr;
    const std::string * mSelectedShaderSource = nullptr;

    Material mHighlightMaterial;

    const Storage & mStorage;
};


} // namespace ad::renderer