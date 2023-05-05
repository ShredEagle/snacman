#include "Rigging.h"

#include <math/Transformations.h>
#include <math/Interpolation/Interpolation.h>
#include <math/Interpolation/QuaternionInterpolation.h>


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


    math::AffineMatrix<4, float> computeMatrix(
        const NodeAnimation::NodeKeyframes & aKeyframes,
        std::size_t aPreviousStep, std::size_t aNextStep,
        float aInterpolant)
    {
        return 
            math::trans3d::scale(
                math::lerp(aKeyframes.mScales[aPreviousStep].as<math::Size>(),
                           aKeyframes.mScales[aNextStep].as<math::Size>(),
                           aInterpolant))
            * math::slerp(aKeyframes.mRotations[aPreviousStep],
                          aKeyframes.mRotations[aNextStep],
                          aInterpolant).toRotationMatrix()
            * math::trans3d::translate(
                math::lerp(aKeyframes.mTranslations[aPreviousStep],
                           aKeyframes.mTranslations[aNextStep],
                           aInterpolant))
            ;
    }

} // namespace unnamed


void NodeAnimation::animate(float aTimepoint, NodeTree<math::AffineMatrix<4, float>> & aNodeTree) const
{
    assert(mTimepoints.size() > 0);

    //
    // Find the indices of the timepoints around the current time,
    // and the interpolation value.
    //
    auto timepointAfter =
        std::find_if(mTimepoints.begin(), mTimepoints.end(),
                     [aTimepoint](float aTime){return aTime > aTimepoint;});
    
    std::size_t previousIdx, nextIdx;
    float interpolant = 0.f;
    if (timepointAfter == mTimepoints.begin())
    {
        previousIdx = nextIdx = 0;
    }
    else if(timepointAfter == mTimepoints.end())
    {
        previousIdx = nextIdx = mTimepoints.size() - 1;
    }
    else
    {
        nextIdx = timepointAfter - mTimepoints.begin();
        previousIdx = nextIdx - 1;
        interpolant = (aTimepoint - mTimepoints[previousIdx]) 
                       / (*timepointAfter - mTimepoints[previousIdx]);
    }

    //
    // Compute the new local pose of each animated node
    //

    // Note: The joint index is the index in the NodeAnimation list of joints.
    // This will be different from the node index in the NodeTree hierarchy.
    for(std::size_t jointIdx = 0; jointIdx != mNodes.size(); ++jointIdx)
    {
        Node::Index hierarchyIdx = mNodes[jointIdx];
        NodeAnimation::NodeKeyframes keyframes = mKeyframesEachNode[jointIdx];

        aNodeTree.mLocalPose[hierarchyIdx] = 
            computeMatrix(mKeyframesEachNode[jointIdx], previousIdx, nextIdx, interpolant);
    }

    //
    // Traverse the node tree hierarchy to compute the global pose of each node
    //
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