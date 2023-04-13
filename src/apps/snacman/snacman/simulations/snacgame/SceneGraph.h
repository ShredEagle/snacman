#pragma once

#include "component/Geometry.h"

#include <math/Homogeneous.h>
#include <math/Quaternion.h>

#include <entity/Entity.h>
#include <concepts>

namespace ad {
namespace snacgame {
ent::Handle<ent::Entity> insertTransformNode(ent::EntityManager & aWorld,
                                             component::Geometry aGeometry,
                                             ent::Handle<ent::Entity> aParent);
void insertEntityInScene(ent::Handle<ent::Entity> aHandle,
                         ent::Handle<ent::Entity> aParent);
void removeEntityFromScene(ent::Handle<ent::Entity> aHandle);
void transferEntity(ent::Handle<ent::Entity> aHandle, ent::Handle<ent::Entity> aNewParent);
math::Position<3, float> getTranslation(const math::AffineMatrix<4, float> & aMatrix);
float getUniformScale(const math::AffineMatrix<4, float> & aMatrix);
math::Quaternion<float> getRotation(const math::AffineMatrix<4, float> & aMatrix, float aScale);

template<class T_pose>
    concept Decomposition = requires(T_pose a)
    {
        { a.mPosition } -> std::convertible_to<math::Position<3, float>>;
        { a.mScaling } -> std::convertible_to<float>;
        { a.mOrientation } -> std::convertible_to<math::Quaternion<float>>;
    };

template<Decomposition T_pose>
void
decomposeMatrix(const math::AffineMatrix<4, float> & aDecomposableMatrix, T_pose & aGlobPose)
{
    // We only use uniform scaling for our heritable transform matrix
    // So no skew in here
    assert(aDecomposableMatrix.at(0, 3) == 0.f && "Can't decompose a transform matrix with skew");
    assert(aDecomposableMatrix.at(1, 3) == 0.f && "Can't decompose a transform matrix with skew");
    assert(aDecomposableMatrix.at(2, 3) == 0.f && "Can't decompose a transform matrix with skew");

    aGlobPose.mPosition = getTranslation(aDecomposableMatrix);
    aGlobPose.mScaling = getUniformScale(aDecomposableMatrix);
    aGlobPose.mOrientation = getRotation(aDecomposableMatrix, aGlobPose.mScaling);
}

} // namespace snacgame
} // namespace ad
