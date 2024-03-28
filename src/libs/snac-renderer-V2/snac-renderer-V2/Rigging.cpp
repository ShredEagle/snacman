#include "Rigging.h"

#include <math/Transformations.h>
#include <math/Interpolation/QuaternionInterpolation.h>


namespace ad::renderer {


namespace {

    math::AffineMatrix<4, float> computeInterpolatedMatrix(
        const RigAnimation::NodeKeyframes & aKeyframes,
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

} // unnamed namespace


// IMPORTANT: This is not a correct NodeTree, has it does not contain hierarchy info
// We should actually use a distinct type here (i.e. TODO)
Rig::FuturePose_type animate(const RigAnimation & aAnimation,
                             float aTimepoint,
                             const NodeTree<Rig::Pose> & aAnimatedTree)
{
    using Node = NodeTree<Rig::Pose>::Node;

    // The posed nodes are only used for a list of global poses
    // (which has to be in the same order as the Rig.)
    NodeTree<Rig::Pose> posedNodes;
    posedNodes.mGlobalPose.reserve(aAnimatedTree.size());

    // Internal use only:
    // Copy the local poses of the rig:
    // some will be overwritten by the animation, but the one not targeted
    // by the animation should have the rig base value.
    auto localPoses = aAnimatedTree.mLocalPose;

    const auto & timepoints = aAnimation.mTimepoints;
    assert(!timepoints.empty());

    //
    // Find the indices of the timepoints around the current time,
    // and the interpolation value.
    //
    auto timepointAfter =
        std::find_if(timepoints.begin(), timepoints.end(),
                     [aTimepoint](float aTime){return aTime > aTimepoint;});
    
    std::size_t previousIdx, nextIdx;
    float interpolant = 0.f;
    if (timepointAfter == timepoints.begin())
    {
        previousIdx = nextIdx = 0;
    }
    else if(timepointAfter == timepoints.end())
    {
        previousIdx = nextIdx = timepoints.size() - 1;
    }
    else
    {
        nextIdx = timepointAfter - timepoints.begin();
        previousIdx = nextIdx - 1;
        interpolant = (aTimepoint - timepoints[previousIdx]) 
                       / (*timepointAfter - timepoints[previousIdx]);
    }

    //
    // Compute the new local pose of each animated node
    // (Important: This might only touch a subset of the bone hierarchy,
    //  since an animation does not have to touch all bones)
    //
    const auto & nodes = aAnimation.mNodes;
    // Note: The joint index is the index in the RigAnimation list of joints.
    // This will be different from the node index in the NodeTree hierarchy, 
    // this hierarchy index is looked up in RigAnimation.mNodes.
    for(std::size_t jointIdx = 0; jointIdx != nodes.size(); ++jointIdx)
    {
        Node::Index hierarchyIdx = nodes[jointIdx];
        const RigAnimation::NodeKeyframes & keyframes = aAnimation.mKeyframes[jointIdx];

        localPoses[hierarchyIdx] = 
            computeInterpolatedMatrix(keyframes, previousIdx, nextIdx, interpolant);
    }

    //
    // Traverse the *whole* node tree hierarchy to compute the global pose of each node
    //
    for(Node::Index hierarchyIdx = 0; hierarchyIdx != aAnimatedTree.mHierarchy.size(); ++hierarchyIdx)
    {
        if(Node::Index parentIdx = aAnimatedTree.mHierarchy[hierarchyIdx].mParent;
           parentIdx != Node::gInvalidIndex)
        {
            posedNodes.mGlobalPose.push_back(
                localPoses[hierarchyIdx] * posedNodes.mGlobalPose[parentIdx]);
        }
        else
        {
            posedNodes.mGlobalPose.push_back(localPoses[hierarchyIdx]);
        }
    }

    return posedNodes;
}


Rig::MatrixPalette Rig::computeJointMatrices(const FuturePose_type & aPosedNodes) const
{
    std::vector<math::AffineMatrix<4, float>> result;
    result.reserve(countJoints());

    computeJointMatrices(std::back_inserter(result), aPosedNodes);

    return result;
}


} // namespace ad::renderer