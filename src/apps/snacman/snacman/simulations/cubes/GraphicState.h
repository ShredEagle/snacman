#pragma once


#include <math/Angle.h>
#include <math/Color.h>
#include <math/Vector.h>
#include <math/Interpolation/Interpolation.h>

#include <vector>


namespace ad {
namespace cubes {


struct Entity
{
    math::Position<3, float> mPosition_world;
    math::Radian<float> mYAngle;
    math::hdr::Rgba_f mColor;
};


struct Camera
{
    math::Position<3, float> mPosition_world;
};


struct GraphicState
{
    std::vector<Entity> mEntities;    
    Camera mCamera; 
};


// TODO Handle cases where left and right do not have the same entities (i.e. only interpolate the intersection).
inline GraphicState interpolate(const GraphicState & aLeft, const GraphicState & aRight, float aInterpolant)
{
    GraphicState state{
        .mCamera = math::lerp(aLeft.mCamera.mPosition_world, aRight.mCamera.mPosition_world, aInterpolant),
    };

    for(auto leftIt = aLeft.mEntities.begin(), rightIt = aRight.mEntities.begin();
        leftIt != aLeft.mEntities.end();
        ++leftIt, ++rightIt)
    {
        state.mEntities.emplace_back(
            math::lerp(leftIt->mPosition_world, rightIt->mPosition_world,   aInterpolant),
            math::lerp(leftIt->mYAngle,         rightIt->mYAngle,           aInterpolant),
            math::lerp(leftIt->mColor,          rightIt->mColor,            aInterpolant));
    }
    return state;
}

} // namespace cubes
} // namespace ad
