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
#include <snac-renderer-V2/Lights.h>
#include <snac-renderer-V2/Model.h>
#include <snac-renderer-V2/Rigging.h>

#include <snac-renderer-V2/graph/EnvironmentMapping.h>
#include <snac-renderer-V2/graph/text/TextGlsl.h>
// TODO Ad 2024/11/27: #text Remove Font.h include
// I think a well designed visu::Text should not be aware of the Font itself
#include <snac-renderer-V2/graph/text/Font.h>

#include <functional>
#include <vector>


namespace ad {
namespace snacgame {

// TODO Ad 2024/11/27: #text this is where the design of the Font system shows its limitations
// we have to wrape the Font + its Part together so the visual state can easily contain the part
// that will be used by the renderer...
// This would totally disapear if it was redesigned so that each string was a precomputed Part instead 
// of a ClientText. Yet this might cause complications:
// "how could main thread create such part, short of synchronizing a call to render thread?"
struct FontAndPart
{
    renderer::Font mFont;
    renderer::TextPart mPart;
};


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


struct Text
{
    // TODO: #text The "string" should ideally be storad via handle:
    // This way it can be reused (instancing the same string at several poses),
    // and interpolation does not have to make a copy of a data struct.
    // (It will probably be required if ClientText becomes an Part or an Object)
    renderer::ClientText mString;
    renderer::Pose mPose;
    math::hdr::Rgba_f mColor;

    // #text This should not be here, but the design lacked so much foresight
    // We need the Part in the renderer to actually draw the text, and the Part
    // is tighlty coupled to the renderer::Font instance in the current design.
    std::shared_ptr<FontAndPart> mFontRef = nullptr;
};

inline Text interpolate(const Text & aLeftEntity, const Text & aRightEntity, float aInterpolant)
{
    return Text{
        .mString = aLeftEntity.mString,
        .mPose = interpolate(aLeftEntity.mPose,
                             aRightEntity.mPose,
                             aInterpolant),
        .mColor = math::lerp(aLeftEntity.mColor, aRightEntity.mColor, aInterpolant),
        .mFontRef = aLeftEntity.mFontRef,
    };
}

struct GraphicState
{
    static constexpr std::size_t MaxEntityId{2048};

    snac::SparseSet<Entity, MaxEntityId> mEntities;    
    snac::SparseSet<Text, MaxEntityId> mTextWorldEntities;
    // TODO #interpolation Interpolate the camera pose
    renderer::Camera mCamera; 
    renderer::LightsDataUi mLights;
    renderer::Environment mEnvironment;

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
        // We probably do not want to interpolate lights, unlikely this would be noticeable
        .mLights{aRight.mLights},
        .mEnvironment{aRight.mEnvironment},
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
