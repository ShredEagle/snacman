#include "Rigging.h"

#include <math/Transformations.h>


namespace ad {
namespace snac {


namespace {

    math::AffineMatrix<4, float> computeMatrix(
        const NodeAnimation::NodeKeyframes & aKeyframes,
        std::size_t aStepIndex)
    {
        return math::trans3d::scale(aKeyframes.mScales[aStepIndex].as<math::Size>())
                * aKeyframes.mRotations[aStepIndex].toRotationMatrix()
                * math::trans3d::translate(aKeyframes.mTranslations[aStepIndex]);
    }

} // namespace unnamed


void NodeAnimation::animate(float aTimepoint, NodeTree<math::AffineMatrix<4, float>> & aNodeTree) const
{
    // Find the current step and interpolation
    auto stepAfter =
        std::find_if(mTimepoints.begin(), mTimepoints.end(),
                     [aTimepoint](float aTime){return aTime > aTimepoint;});
    
    std::size_t stepIdx = stepAfter != mTimepoints.end() ?
        stepAfter - mTimepoints.begin()
        : mTimepoints.size() - 1;

    // The joint index is the index in the NodeAnimation list of joints.
    // This will be different from the node index in the NodeTree hierarchy.
    for(std::size_t jointIdx = 0; jointIdx != mNodes.size(); ++jointIdx)
    {
        Node::Index hierarchyIdx = mNodes[jointIdx];
        NodeAnimation::NodeKeyframes keyframes = mKeyframesEachNode[jointIdx];

        aNodeTree.mLocalPose[hierarchyIdx] = computeMatrix(mKeyframesEachNode[jointIdx], stepIdx);
    }

    for(Node::Index nodeIdx = 0; nodeIdx != aNodeTree.mHierarchy.size(); ++nodeIdx)
    {
        if(Node::Index parentIdx = aNodeTree.mHierarchy[nodeIdx].mParent;
            parentIdx != Node::gInvalidIndex)
        {
            aNodeTree.mGlobalPose[nodeIdx] =
                aNodeTree.mLocalPose[nodeIdx] * aNodeTree.mGlobalPose[parentIdx];
        }
        else
        {
            aNodeTree.mGlobalPose[nodeIdx] = aNodeTree.mLocalPose[nodeIdx];
        }
    }
}


std::vector<math::AffineMatrix<4, float>> Rig::computeJointMatrices()
{
    std::vector<math::AffineMatrix<4, float>> result;
    result.reserve(mJoints.size());

    for(std::size_t jointIdx = 0; jointIdx != mJoints.size(); ++jointIdx)
    {
        result.push_back(mInverseBindMatrices[jointIdx] * mScene.mGlobalPose[mJoints[jointIdx]]);
    }

    return result;
}


} // namespace snac
} // namespace ad