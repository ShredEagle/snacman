#pragma once


#include <math/Homogeneous.h>

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
    Node::Index mFirstRoot = Node::gInvalidIndex;

    // TODO can we do without storing the local pose for skeletal animation?
    // (might not be the case when interpolating between several animations)
    std::vector<T_Pose> mLocalPose;
    std::vector<T_Pose> mGlobalPose;

    std::unordered_map<typename Node::Index, std::string> mNodeNames;
};



/// @brief 
/// @note A single Rig might be used for several Parts of a given Object.
struct Rig
{
    using Pose = math::AffineMatrix<4, float>;
    using Ibm = math::AffineMatrix<4, float>;

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


/// @brief Per-vertex data required for skeletal animation.
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