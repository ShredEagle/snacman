#pragma once


#include "../../SparseSet.h"

#include <math/Angle.h>
#include <math/Color.h>
#include <math/Homogeneous.h> // TODO #pose remove
#include <math/Vector.h>
#include <math/Interpolation/Interpolation.h>
#include <math/Interpolation/QuaternionInterpolation.h>

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
};

                    
inline Entity interpolate(const Entity & aLeftEntity, const Entity & aRightEntity, float aInterpolant)
{
    return Entity{
        math::lerp(aLeftEntity.mPosition_world, aRightEntity.mPosition_world,   aInterpolant),
        math::lerp(aLeftEntity.mScaling,        aRightEntity.mScaling,          aInterpolant),
        math::slerp(aLeftEntity.mOrientation,   aRightEntity.mOrientation,      aInterpolant),
        math::lerp(aLeftEntity.mColor,          aRightEntity.mColor,            aInterpolant),
    };
}


// TODO #generic-render We should watch out for the proliferation of specialized graphics state entities,
// which is by definition the opposite of genericity.
struct TextScreen
{
    math::Position<2, float> mPosition_unitscreen;
    math::Radian<float> mOrientation;
    std::string mString;
    std::shared_ptr<snac::Font> mFont;
    math::hdr::Rgba_f mColor;
};


inline TextScreen interpolate(const TextScreen & aLeftEntity, const TextScreen & aRightEntity, float aInterpolant)
{
    return TextScreen{
        .mPosition_unitscreen = math::lerp(aLeftEntity.mPosition_unitscreen,
                                           aRightEntity.mPosition_unitscreen,
                                           aInterpolant),
        .mOrientation = math::lerp(aLeftEntity.mOrientation, aRightEntity.mOrientation, aInterpolant),
        .mString = aLeftEntity.mString,
        .mFont = aLeftEntity.mFont,
        .mColor = math::lerp(aLeftEntity.mColor, aRightEntity.mColor, aInterpolant),
    };
}


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

    snac::SparseSet<TextScreen, MaxEntityId> mTextEntities;

    std::vector<std::function<void()>> mImguiCommands;
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
        .mImguiCommands{aRight.mImguiCommands},
    };

    interpolateEach(aInterpolant, aLeft.mEntities, aRight.mEntities, state.mEntities);

    interpolateEach(aInterpolant, aLeft.mTextEntities, aRight.mTextEntities, state.mTextEntities);


    return state;
}


} // namespace visu
} // namespace cubes
} // namespace ad
