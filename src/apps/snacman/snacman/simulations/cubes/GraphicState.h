#pragma once


#include <math/Angle.h>
#include <math/Color.h>
#include <math/Homogeneous.h> // TODO #pose remove
#include <math/Vector.h>
#include <math/Interpolation/Interpolation.h>

#include <vector>


namespace ad {
namespace cubes {
namespace visu {


struct Entity
{
    math::Position<3, float> mPosition_world;
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
    std::vector<Entity> mEntities;    
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

    for(auto leftIt = aLeft.mEntities.begin(), rightIt = aRight.mEntities.begin();
        leftIt != aLeft.mEntities.end();
        ++leftIt, ++rightIt)
    {
        state.mEntities.push_back(Entity{
            math::lerp(leftIt->mPosition_world, rightIt->mPosition_world,   aInterpolant),
            math::lerp(leftIt->mYAngle,         rightIt->mYAngle,           aInterpolant),
            math::lerp(leftIt->mColor,          rightIt->mColor,            aInterpolant)});
    }
    return state;
}

} // namespace visu
} // namespace cubes
} // namespace ad
