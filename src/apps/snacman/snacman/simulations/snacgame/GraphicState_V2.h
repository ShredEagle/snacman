#pragma once


#include "Handle_V2.h"
#include "../../SparseSet.h"

#include <math/Angle.h>
#include <math/Color.h>
#include <math/Homogeneous.h> // TODO #pose remove
#include <math/Vector.h>
#include <math/Interpolation/Interpolation.h>
#include <math/Interpolation/QuaternionInterpolation.h>

// TODO remove V1 includes
#include <snac-renderer-V1/DebugDrawer.h>
#include <snac-renderer-V1/text/Text.h>

#include <snac-renderer-V2/Camera.h>
#include <snac-renderer-V2/Model.h>
#include <snac-renderer-V2/Rigging.h>

#include <functional>
#include <vector>


namespace ad {
namespace snacgame {
namespace visu_V2 {


struct Entity
{
    math::hdr::Rgba_f mColor;
    // Note: the Pose is embedded in the Instance
    renderer::Instance mInstance;
    // Potentially non-uniform scaling, that cannot induce skewing
    // because the Entity are leaves (they cannot have children in a scene graph)
    math::Size<3, GLfloat> mInstanceScaling;

    // TODO #animation would be better to interpolate the animation time (Parameter)
    // between each GPU frame, instead of providing fixed parameter value
    // (cannot be inteprolated because of periodic behaviours).
    struct SkeletalAnimation
    {
        renderer::Handle<const renderer::RigAnimation> mAnimation = renderer::gNullHandle;
        float mParameterValue = 0.;
    } mAnimationState;

    bool mDisableInterpolation = false;
};

                    
inline Entity interpolate(const Entity & aLeftEntity, const Entity & aRightEntity, float aInterpolant)
{
    if(aRightEntity.mDisableInterpolation)
    {
        return aRightEntity;
    }
    else
    {
        Entity result{
            math::lerp(aLeftEntity.mColor, aRightEntity.mColor, aInterpolant),
            aRightEntity.mInstance,
            math::lerp(aLeftEntity.mInstanceScaling, aRightEntity.mInstanceScaling, aInterpolant),
            aRightEntity.mAnimationState,
        };
        result.mInstance.mPose = interpolate(aLeftEntity.mInstance.mPose, aRightEntity.mInstance.mPose, aInterpolant);
        return result;
    }
}

} // namespace visu_V2


namespace visu_V1 {


struct Text
{
    math::Position<3, float> mPosition_world;
    math::Size<3, float> mScaling; // TODO were is it used?
    math::Quaternion<float> mOrientation;
    std::string mString;
    std::shared_ptr<snac::Font> mFont;
    math::hdr::Rgba_f mColor;
};

inline Text interpolate(const Text & aLeftEntity, const Text & aRightEntity, float aInterpolant)
{
    return Text{
        .mPosition_world = math::lerp(aLeftEntity.mPosition_world,
                                      aRightEntity.mPosition_world,
                                      aInterpolant),
        .mScaling = math::lerp(aLeftEntity.mScaling, aRightEntity.mScaling, aInterpolant),
        .mOrientation = math::slerp(aLeftEntity.mOrientation, aRightEntity.mOrientation, aInterpolant),
        .mString = aLeftEntity.mString,
        .mFont = aLeftEntity.mFont,
        .mColor = math::lerp(aLeftEntity.mColor, aRightEntity.mColor, aInterpolant),
    };
}


} // namespace visu_V1


namespace visu_V2 {


struct GraphicState
{
    static constexpr std::size_t MaxEntityId{2048};

    snac::SparseSet<Entity, MaxEntityId> mEntities;    
    snac::SparseSet<visu_V1::Text, MaxEntityId> mTextWorldEntities;
    // TODO #interpolation Interpolate the camera pose
    renderer::Camera mCamera; 

    snac::SparseSet<visu_V1::Text, MaxEntityId> mTextScreenEntities;

    snac::DebugDrawer::DrawList mDebugDrawList;
};


template <class T_entity, std::size_t N_universeSize>
void interpolateEach(float aInterpolant,
                     const snac::SparseSet<T_entity, N_universeSize> & aLeftSet,
                     const snac::SparseSet<T_entity, N_universeSize> & aRightSet,
                     snac::SparseSet<T_entity, N_universeSize> & aOutSet)
{
    for(const auto & [id, rightEntity] : aRightSet)
    {
        if(aLeftSet.contains(id)) 
        {
            const T_entity & leftEntity = aLeftSet[id];    
            aOutSet.insert(id, interpolate(leftEntity, rightEntity, aInterpolant));
        }
    }
}

// TODO Handle cases where left and right do not have the same entities (i.e. only interpolate the sets intersection).
inline GraphicState interpolate(const GraphicState & aLeft, const GraphicState & aRight, float aInterpolant)
{
    GraphicState state{
        // TODO #pose
        //.mCamera = math::lerp(aLeft.mCamera.mPosition_world, aRight.mCamera.mPosition_world, aInterpolant),
        .mCamera{aRight.mCamera},
        // Note: For the moment, we do not interpolate the debug drawings, both for simplicity and performance
        .mDebugDrawList{aRight.mDebugDrawList},
    };

    interpolateEach(aInterpolant, aLeft.mEntities, aRight.mEntities, state.mEntities);

    interpolateEach(aInterpolant, aLeft.mTextWorldEntities, aRight.mTextWorldEntities, state.mTextWorldEntities);

    interpolateEach(aInterpolant, aLeft.mTextScreenEntities, aRight.mTextScreenEntities, state.mTextScreenEntities);

    return state;
}


} // namespace visu_V2
} // namespace cubes
} // namespace ad
