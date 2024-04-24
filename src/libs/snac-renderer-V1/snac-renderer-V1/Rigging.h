#pragma once


#include <arte/gltf/Gltf-decl.h>

#include <vector>

#include <cassert>
#include <cstddef>


namespace ad {
namespace snac {


struct Node
{   
    using Index = std::size_t;
    static constexpr Index gInvalidIndex = std::numeric_limits<Index>::max();

    Index mParent      = gInvalidIndex;
    Index mFirstChild  = gInvalidIndex;
    Index mNextSibling = gInvalidIndex;
    Index mLastSibling = gInvalidIndex; // WARNING: this is an inconsistent internal value, not an invariant.
    unsigned int mLevel = 0;
};


template <class T_Pose>
struct NodeTree
{
    /// @param aParent if set to invalid index, the call adds a root node.
    Node::Index addNode(Node::Index aParent, T_Pose aPose);

    bool hasParent(Node::Index aNode) const
    { return mHierarchy[aNode].mParent != Node::gInvalidIndex; }

    bool hasChild(Node::Index aNode) const
    { return mHierarchy[aNode].mFirstChild != Node::gInvalidIndex; }

    bool hasName(Node::Index aNode) const
    { return mNodeNames.find(aNode) != mNodeNames.end(); }

    void reserve(std::size_t aCapacity)
    {
        mHierarchy.reserve(aCapacity);
        mLocalPose.reserve(aCapacity);
        mGlobalPose.reserve(aCapacity);
    }

    std::vector<Node> mHierarchy;
    // TODO can we do without storing the local pose for skeletal animation?
    // (might not be the case when interpolating between several animations)
    std::vector<T_Pose> mLocalPose;
    std::vector<T_Pose> mGlobalPose;

    Node::Index mFirstRoot = Node::gInvalidIndex;

    std::unordered_map<Node::Index, std::string> mNodeNames;
};


//template <class T_Pose>
// TODO rename (clarify the fact it is not the animation of just a single node)
struct NodeAnimation
{
    // TODO #anim hosting the NodeTree in the Rig instance is a problem,
    // notably because this function mutates the node tree directly.
    // see also todo (a48c8) 
    // Maybe the Rig should host a read only hierarchy (no transformation) of joints (no whole scene)
    // and the animation would produce an array of global transformations from this hierarchy.
    void animate(float aTimepoint, NodeTree<math::AffineMatrix<4, float>> & aNodeTree) const;

    // The sequence of node indices that are animated.
    // (This is the order in which node pose data is provided.)
    // Note: these are the indices of node entries in animate()'s `aNodeTree`.
    std::vector<Node::Index> mNodes;

    // Ideally, keyframes would be on a constant period, instead of an of time points.
    // This would be more performant to find the current keyframe indices.
    std::vector<float> mTimepoints;
    float mEndTime;

    // We are taking the approach array-of-struct, storing each pose (struct) 
    // for a given joint in the array of all keyframes. (storing each joint in an array on top).
    // Yet, it might be even more cache-friendly to tightly pack all poses 
    // for all joints at a given tick in the innermost array, and store each tick in an array above.
    // (should definitely be measured before refactoring).
    //using NodeKeyframes = std::vector<T_Pose>;
    struct NodeKeyframes
    {
        std::vector<math::Vec<3, float>> mTranslations;
        std::vector<math::Quaternion<float>> mRotations;
        std::vector<math::Vec<3, float>> mScales;
    };
    std::vector<NodeKeyframes> mKeyframesEachNode;
};


struct Rig
{
    using Pose = math::AffineMatrix<4, float>;

    std::vector<math::AffineMatrix<4, float>> computeJointMatrices();

    // TODO #anim Ideally, the nodetree would also provide the order of joints
    // and this member can be removed.
    std::vector<Node::Index> mJoints;
    // TODO #anim should only load the joints instead of the whole scene.
    // (potential complication if there are non-joints between joints)
    NodeTree<Pose> mScene;
    std::vector<math::AffineMatrix<4, float>> mInverseBindMatrices;
    std::string mName;
};


//
// Implementation
//
template <class T_Pose>
std::ostream & operator<<(std::ostream & aOut, const NodeTree<T_Pose> & aTree)
{
    std::function<void(Node::Index)> dump = [&aOut, &aTree, &dump](Node::Index aIdx)
    {
        static const std::string gDefaultName{"<UNNAMED>"};

        const Node & node = aTree.mHierarchy[aIdx];
        std::fill_n(std::ostreambuf_iterator<char>{aOut}, 2*node.mLevel, ' ');
        
        aOut << "(idx: " << aIdx << ")"
             << (aTree.mNodeNames.count(aIdx) == 1 ? aTree.mNodeNames.at(aIdx) : gDefaultName)
             ;

        for(Node::Index childIdx = node.mFirstChild;
            childIdx != Node::gInvalidIndex;
            childIdx = aTree.mHierarchy[childIdx].mNextSibling)
        {
            aOut << "\n";
            dump(childIdx);
        }
    };

    // Do **not** prepend the very first output with a '\n', but do prepend each following print.
    // This implements the "join" behaviour, '\n' only being present between elements.
    dump(aTree.mFirstRoot);
    for(Node::Index rootIdx = aTree.mHierarchy[aTree.mFirstRoot].mNextSibling;
        rootIdx != Node::gInvalidIndex;
        rootIdx = aTree.mHierarchy[rootIdx].mNextSibling)
    {
        aOut << "\n";
        dump(rootIdx);
    }

    return aOut;
}

template <class T_Pose>
Node::Index NodeTree<T_Pose>::addNode(Node::Index aParent, T_Pose aPose)
{
    Node::Index thisIndex = mHierarchy.size();

    mHierarchy.push_back(Node{
        .mParent = aParent,
    });

    mLocalPose.push_back(std::move(aPose));

    Node & node = mHierarchy[thisIndex];

    if(aParent != Node::gInvalidIndex) // This node has a parent
    {
        Node & parent = mHierarchy[aParent];
        node.mLevel = parent.mLevel + 1;
        mGlobalPose.push_back(mLocalPose[thisIndex] * mGlobalPose[aParent]);

        if (parent.mFirstChild == Node::gInvalidIndex) // The parent has no child
        {
            parent.mFirstChild = thisIndex;
            node.mLastSibling = thisIndex;
        }
        else // The parent has children
        {
            Node & firstSibling = mHierarchy[parent.mFirstChild];
            Node::Index currentLastIdx = firstSibling.mLastSibling;
            // If the first sibling does not have its last sibling value set,
            // find the last sibling by traversing the siblings.
            if(currentLastIdx == Node::gInvalidIndex)
            {
                for(currentLastIdx = parent.mFirstChild;
                    mHierarchy[currentLastIdx].mNextSibling != Node::gInvalidIndex;
                    currentLastIdx = mHierarchy[currentLastIdx].mNextSibling)    
                {}
            }
            mHierarchy[currentLastIdx].mNextSibling = thisIndex;
            firstSibling.mLastSibling = thisIndex;
        }
    }
    else // This is a root node
    {
        mGlobalPose.push_back(mLocalPose[thisIndex]);

        if (mFirstRoot != Node::gInvalidIndex) // This is not the first root node
        {
            assert(mHierarchy[mFirstRoot].mLastSibling != Node::gInvalidIndex);
            Node::Index currentLastIdx = mHierarchy[mFirstRoot].mLastSibling;

            mHierarchy[currentLastIdx].mNextSibling = thisIndex;
        }
        else // This is the first root node
        {
            mFirstRoot = thisIndex;
        }
        mHierarchy[mFirstRoot].mLastSibling = thisIndex;
    }

    return thisIndex;
}



} // namespace snac
} // namespace ad