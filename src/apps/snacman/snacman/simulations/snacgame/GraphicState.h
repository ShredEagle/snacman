#pragma once


#include "../../SparseSet.h"

#include <functional>
#include <math/Angle.h>
#include <math/Color.h>
#include <math/Homogeneous.h> // TODO #pose remove
#include <math/Vector.h>
#include <math/Interpolation/Interpolation.h>

#include <vector>


namespace ad {
namespace snacgame {
namespace visu {


struct Entity
{
    math::Position<3, float> mPosition_world;
    math::Size<3, float> mScaling;
    math::Radian<float> mYAngle;
    math::hdr::Rgba_f mColor;
};


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
    Camera mCamera; 
};


// TODO Handle cases where left and right do not have the same entities (i.e. only interpolate the sets intersection).
inline GraphicState interpolate(const GraphicState & aLeft, const GraphicState & aRight, float aInterpolant)
{
    GraphicState state{
        // TODO #pose
        //.mCamera = math::lerp(aLeft.mCamera.mPosition_world, aRight.mCamera.mPosition_world, aInterpolant),
        .mCamera{aRight.mCamera},
    };

    for(const auto & [id, rightEntity] : aRight.mEntities)
    {
        if(aLeft.mEntities.contains(id)) 
        {
            const Entity & leftEntity = aLeft.mEntities[id];    
            state.mEntities.insert(
                id,
                Entity{
                    math::lerp(leftEntity.mPosition_world, rightEntity.mPosition_world,   aInterpolant),
                    math::lerp(leftEntity.mScaling,        rightEntity.mScaling,   aInterpolant),
                    math::lerp(leftEntity.mYAngle,         rightEntity.mYAngle,           aInterpolant),
                    math::lerp(leftEntity.mColor,          rightEntity.mColor,            aInterpolant)});
        }
    }
    return state;
}

} // namespace visu
} // namespace cubes
} // namespace ad
