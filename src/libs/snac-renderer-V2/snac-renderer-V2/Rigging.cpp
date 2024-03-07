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


void animate(const RigAnimation & aAnimation, float aTimepoint, NodeTree<Rig::Pose> & aAnimatedTree)
{
    using Node = NodeTree<Rig::Pose>::Node;

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

        aAnimatedTree.mLocalPose[hierarchyIdx] = 
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
            aAnimatedTree.mGlobalPose[hierarchyIdx] =
                aAnimatedTree.mLocalPose[hierarchyIdx] * aAnimatedTree.mGlobalPose[parentIdx];
        }
        else
        {
            aAnimatedTree.mGlobalPose[hierarchyIdx] = aAnimatedTree.mLocalPose[hierarchyIdx];
        }
    }
}


} // namespace ad::renderer