#pragma once


#include "../../SparseSet.h"

#include <math/Angle.h>
#include <math/Color.h>
#include <math/Homogeneous.h> // TODO #pose remove
#include <math/Vector.h>
#include <math/Interpolation/Interpolation.h>
#include <math/Interpolation/QuaternionInterpolation.h>

#include <snac-renderer/DebugDrawer.h>
#include <snac-renderer/Rigging.h>
#include <snac-renderer/text/Text.h>

#include <functional>
#include <vector>


namespace ad {
namespace snacgame {
namespace visu {


struct Entity
{
    math::Position<3, float> mPosition_world;
    math::Size<3, float> mScaling; // TODO were is it used?
    math::Quaternion<float> mOrientation;
    math::hdr::Rgba_f mColor;
    std::shared_ptr<snac::Model> mModel;

    // TODO #anim would be better to interpolate the animation time (Parameter)
    // between each GPU frame, instead of providing fixed parameter value
    // (cannot be inteprolated because of periodic behaviours).
    struct SkeletalAnimation
    {
        //TODO #anim (a48c8) this is **not** const because animation writes to the underlying scene,
        // but this is very smelly (and an invitatin to race conditions)
        snac::Rig *mRig = nullptr;
        const snac::NodeAnimation * mAnimation = nullptr;
        double mParameterValue = 0.;
    } mRigging;

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
        return Entity{
            math::lerp(aLeftEntity.mPosition_world, aRightEntity.mPosition_world,   aInterpolant),
            math::lerp(aLeftEntity.mScaling,        aRightEntity.mScaling,          aInterpolant),
            math::slerp(aLeftEntity.mOrientation,   aRightEntity.mOrientation,      aInterpolant),
            math::lerp(aLeftEntity.mColor,          aRightEntity.mColor,            aInterpolant),
            aRightEntity.mModel,
            aRightEntity.mRigging,
        };
    }
}


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


// TODO #camera can we use a complete snac::Camera instead?
//      This way the Renderer gets the complete camera each frame.
struct Camera
{
    // TODO #pose replace with position and orientation
    math::AffineMatrix<4, float> mWorldToCamera{math::AffineMatrix<4, float>::Identity()};

    //math::Position<3, float> mPosition_world;
};


struct GraphicState
{
    static constexpr std::size_t MaxEntityId{2048};

    snac::SparseSet<Entity, MaxEntityId> mEntities;    
    snac::SparseSet<Text, MaxEntityId> mTextWorldEntities;
    Camera mCamera; 

    snac::SparseSet<Text, MaxEntityId> mTextScreenEntities;

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


} // namespace visu
} // namespace cubes
} // namespace ad
