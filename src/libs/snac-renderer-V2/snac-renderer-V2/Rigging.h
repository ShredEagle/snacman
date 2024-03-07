#pragma once


#include "Commons.h"

#include <math/Homogeneous.h>
#include <math/Quaternion.h>

#include <functional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <cassert>


namespace ad::renderer {


template <class T_Pose>
struct NodeTree
{

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

    using Callback_t = std::function<void(typename NodeTree::Node::Index /*node*/,
                                          NodeTree &/*tree*/)>;

    /// @param aParent if set to invalid index, the call adds a root node.
    Node::Index addNode(Node::Index aParent, T_Pose aPose);

    bool hasParent(Node::Index aNode) const
    { return mHierarchy[aNode].mParent != Node::gInvalidIndex; }

    bool hasChild(Node::Index aNode) const
    { return mHierarchy[aNode].mFirstChild != Node::gInvalidIndex; }

    bool hasName(Node::Index aNode) const
    { return mNodeNames.find(aNode) != mNodeNames.end(); }

    Node & operator[](Node::Index aNodeIdx)
    { return mHierarchy[aNodeIdx]; }

    const Node & operator[](Node::Index aNodeIdx) const
    { return mHierarchy[aNodeIdx]; }

    void reserve(std::size_t aCapacity)
    {
        mHierarchy.reserve(aCapacity);
        mLocalPose.reserve(aCapacity);
        mGlobalPose.reserve(aCapacity);
    }

    std::vector<Node> mHierarchy;
    Node::Index mFirstRoot = Node::gInvalidIndex;

    // TODO can we do without storing the local pose for skeletal animation?
    // (might not be the case when interpolating between several animations)
    std::vector<T_Pose> mLocalPose;
    std::vector<T_Pose> mGlobalPose;

    std::unordered_map<typename Node::Index, std::string> mNodeNames;
};


/// @brief Depth first traversal.
template <class T_pose>
void traverseDepth(NodeTree<T_pose> & aTree,
                   const typename NodeTree<T_pose>::Callback_t & aCallback)
{
    traverseDepth(aTree, aCallback, aTree.mFirstRoot);
}
                   
template <class T_pose>
void traverseDepth(NodeTree<T_pose> & aTree,
                   const typename NodeTree<T_pose>::Callback_t & aCallback,
                   typename NodeTree<T_pose>::Node::Index aNodeIdx)
{
    using Node = NodeTree<T_pose>::Node;

    aCallback(aNodeIdx, aTree);

    const Node & node = aTree.mHierarchy[aNodeIdx];

    if(aTree.hasChild(aNodeIdx))
    {
        traverseDepth(aTree, aCallback, node.mFirstChild);
    }

    if(node.mNextSibling != Node::gInvalidIndex)
    {
        traverseDepth(aTree, aCallback, node.mNextSibling);
    }
}


/// @brief Attempt to model a notion of Skeleton / Rig (this notion is not an explicit concept in Assimp).
/// @note A single Rig might be used for several Parts of a given Object.
struct Rig
{
    using Pose = math::AffineMatrix<4, float>;
    using Ibm = math::AffineMatrix<4, float>;

    /// @brief Return the joint matrix palette, used by shaders to compute vertex deplacement
    /// from skeletal animation.
    std::vector<Rig::Pose> computeJointMatrices() const;

    // Note: currently usually represents a hierarchy larger than the skeleton.
    // The entries of this tree that are used as actual bones are given by mJoints.mIndices.
    NodeTree<Pose> mJointTree;

    struct JointData
    {
        // Maps a bone index in aiMesh.mBones[] (the index into mJoints array) 
        // to a Node index in mJointTree (the value)
        // TODO #anim Ideally, mJointsTree nodes would be "bones" only,
        // indexed in the same order as bone indices provided as a vertex attribute,
        // so this member could be removed (getting rid of one indirection)
        std::vector<NodeTree<Pose>::Node::Index> mIndices;

        // Directly given in the order of bones in aiMesh.mBones.
        std::vector<Ibm> mInverseBindMatrices;
    } mJoints;

    // With Assimp, I do not know any better name for the rig as a whole than its armature
    std::string mArmatureName;
};


struct RigAnimation
{
    std::string mName; // Might duplicate some keys in the map of animation at first, but the key might become a hash.

    float mDuration;

    // SOA design for animation keyframes
    std::vector<float> mTimepoints;

    // Attention: This data members couples the RigAnimation to the Rig, which is a bit smelly.
    // I do not know if we want this class to become a member of the Rig, so coupling is assumed.
    // The sequence of nodes that are animated, given by index in the Rig's NodeTree.
    // Note: It would be tempting to get rid of those indices, by giving the animation in the 
    // (linear) order of nodes in the Rig's NodeTree. Yet it is common that somes nodes are not animated.
    // (It might still be better for spatial coherance to sort those indices).
    std::vector<NodeTree<Rig::Pose>::Node::Index> mNodes;

    struct NodeKeyframes
    {
        std::vector<math::Vec<3, float>> mTranslations;
        std::vector<math::Quaternion<float>> mRotations;
        std::vector<math::Vec<3, float>> mScales;

        void reserve(std::size_t aCapacity)
        {
            mTranslations.reserve(aCapacity);
            mRotations.reserve(aCapacity);
            mScales.reserve(aCapacity);
        }

    };
    // The keyframes for each node, given in the order of mNodes.
    std::vector<NodeKeyframes> mKeyframes;
};


void animate(const RigAnimation & aAnimation, float aTimepoint, NodeTree<Rig::Pose> & aAnimatedTree);


/// @brief Associated RigAnimations to the Rig they target
struct AnimatedRig
{
    Rig mRig;
    std::unordered_map<StringKey/*name*/, RigAnimation> mNameToAnimation;
};


/// @brief Per-vertex data required for skeletal animation, usually used as generic vertex attribute.
struct VertexJointData
{
    static constexpr std::size_t gMaxBones = 4;
    using BoneIndex_t = unsigned int; // TODO #perf use a more compact type than uint

    // Would require attribute interleaving, and VertexJointData would be per-vertex 
    // (instead of containing all vertices)
    //math::Vec<gMaxBones, unsigned int> mBoneIndices{math::Vec<gMaxBones, unsigned int>::Zero()};
    //math::Vec<gMaxBones, float> mBoneWeights{math::Vec<gMaxBones, float>::Zero()};
    std::vector<math::Vec<gMaxBones, BoneIndex_t>> mBoneIndices; 
    std::vector<math::Vec<gMaxBones, float>> mBoneWeights;
};


//
// Implementation
//
template <class T_Pose>
std::ostream & operator<<(std::ostream & aOut, const NodeTree<T_Pose> & aTree)
{
    using Node = NodeTree<T_Pose>::Node;

    std::function<void(typename Node::Index)> dump = [&aOut, &aTree, &dump](Node::Index aIdx)
    {
        static const std::string gDefaultName{"<UNNAMED>"};

        const Node & node = aTree.mHierarchy[aIdx];
        std::fill_n(std::ostreambuf_iterator<char>{aOut}, 2*node.mLevel, ' ');
        
        aOut << "(idx: " << aIdx << ")"
             << (aTree.mNodeNames.count(aIdx) == 1 ? aTree.mNodeNames.at(aIdx) : gDefaultName)
             ;

        for(typename Node::Index childIdx = node.mFirstChild;
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
    for(typename Node::Index rootIdx = aTree.mHierarchy[aTree.mFirstRoot].mNextSibling;
        rootIdx != Node::gInvalidIndex;
        rootIdx = aTree.mHierarchy[rootIdx].mNextSibling)
    {
        aOut << "\n";
        dump(rootIdx);
    }

    return aOut;
}

template <class T_Pose>
NodeTree<T_Pose>::Node::Index NodeTree<T_Pose>::addNode(Node::Index aParent, T_Pose aPose)
{
    typename Node::Index thisIndex = mHierarchy.size();

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
            typename Node::Index currentLastIdx = firstSibling.mLastSibling;
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
            typename Node::Index currentLastIdx = mHierarchy[mFirstRoot].mLastSibling;

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


} // namespace ad::renderer