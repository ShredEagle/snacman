#include "SceneGraph.h"

#include "component/Geometry.h"
#include "component/GlobalPose.h"
#include "component/SceneNode.h"
#include "typedef.h"

#include <entity/EntityManager.h>
#include <math/Transformations.h>

namespace ad {
namespace snacgame {

Pos3 getTranslation(const TransformMatrix & aMatrix)
{
    return Pos3{aMatrix.at(3, 0), aMatrix.at(3, 1), aMatrix.at(3, 2)};
}

float getUniformScale(const TransformMatrix & aMatrix)
{
    // Scale is always uniform in the transform matrix passed down
    // in the scene graph so we only need to compute one of the
    // component of the scale
    Vec3 scale = {aMatrix.at(0, 0), aMatrix.at(1, 0), aMatrix.at(2, 0)};
    return scale.getNorm();
}

Quat_f getRotation(const TransformMatrix & aMatrix, float aScale)
{
    Matrix3 rot = aMatrix.getSubmatrix(3, 3) / aScale;
    // From https://gamemath.com/book/orient.html#matrix_to_quaternion_values
    // as reference in in GPU pro 360 15.2.4
    // be careful mathematician index matrix from 1 to 3 not from 0 to 2
    float fourWSquaredMinus1 = rot.trace();
    float fourXSquaredMinus1 = rot.at(0, 0) - rot.at(1, 1) - rot.at(2, 2);
    float fourYSquaredMinus1 = rot.at(1, 1) - rot.at(2, 2) - rot.at(0, 0);
    float fourZSquaredMinus1 = rot.at(2, 2) - rot.at(0, 0) - rot.at(1, 1);

    int biggestIndex = 0;
    float fourBiggestSquared = fourWSquaredMinus1;

    if (fourBiggestSquared < fourXSquaredMinus1)
    {
        fourBiggestSquared = fourXSquaredMinus1;
        biggestIndex = 1;
    }
    if (fourBiggestSquared < fourYSquaredMinus1)
    {
        fourBiggestSquared = fourYSquaredMinus1;
        biggestIndex = 2;
    }
    if (fourBiggestSquared < fourZSquaredMinus1)
    {
        fourBiggestSquared = fourZSquaredMinus1;
        biggestIndex = 3;
    }

    float biggestVal = std::sqrt(fourBiggestSquared + 1.f) / 2.f;
    float mult = 0.25f / biggestVal;

    float w = 1;
    float x = 0;
    float y = 0;
    float z = 0;
    switch (biggestIndex)
    {
    case 0:
        w = biggestVal;
        x = (rot.at(1, 2) - rot.at(2, 1)) * mult;
        y = (rot.at(2, 0) - rot.at(0, 2)) * mult;
        z = (rot.at(0, 1) - rot.at(1, 0)) * mult;
        break;
    case 1:
        x = biggestVal;
        w = (rot.at(1, 2) - rot.at(2, 1)) * mult;
        y = (rot.at(0, 1) + rot.at(1, 0)) * mult;
        z = (rot.at(2, 0) + rot.at(0, 2)) * mult;
        break;
    case 2:
        y = biggestVal;
        w = (rot.at(2, 0) - rot.at(0, 2)) * mult;
        x = (rot.at(0, 1) + rot.at(1, 0)) * mult;
        z = (rot.at(1, 2) + rot.at(2, 1)) * mult;
        break;
    case 3:
        z = biggestVal;
        w = (rot.at(0, 1) - rot.at(1, 0)) * mult;
        y = (rot.at(1, 2) + rot.at(2, 1)) * mult;
        x = (rot.at(2, 0) + rot.at(0, 2)) * mult;
        break;
    }
    return Quat_f(x, y, z, w);
}

namespace {
void computeNewLocalTransform(component::Geometry & aChildGeo,
                              const component::GlobalPose & aChildPose,
                              const component::GlobalPose & aParentPose)
{
    TransformMatrix childTransform =
        math::trans3d::scaleUniform(aChildPose.mScaling)
        * aChildPose.mOrientation.toRotationMatrix()
        * math::trans3d::translate(aChildPose.mPosition.as<math::Vec>());
    TransformMatrix invParentTransform =
        (math::trans3d::scaleUniform(aParentPose.mScaling)
         * aParentPose.mOrientation.toRotationMatrix()
         * math::trans3d::translate(aParentPose.mPosition.as<math::Vec>()))
            .inverse();

    TransformMatrix newLocalTransform = childTransform * invParentTransform;

    decomposeMatrix(newLocalTransform, aChildGeo);
}
} // namespace

// Allows to easily insert a node with a transformation into the scene graph
EntHandle insertTransformNode(ent::EntityManager & aWorld,
                              component::Geometry aGeometry,
                              ent::Handle<ent::Entity> aParent)
{
    EntHandle nodeEnt = aWorld.addEntity();

    {
        ent::Phase graphPhase;

        nodeEnt.get(graphPhase)
            ->add(aGeometry)
            .add(component::SceneNode{})
            .add(component::GlobalPose{});
    }

    insertEntityInScene(nodeEnt, aParent);

    return nodeEnt;
}


void insertEntityInScene(ent::Handle<ent::Entity> aHandle,
                         ent::Handle<ent::Entity> aParent)
{
    ent::Phase graphPhase;
    Entity parentEntity = *aParent.get(graphPhase);
    // TODO Ad: Dayum, we need typed handles...
    assert(parentEntity.has<component::SceneNode>()
           && "Can't add a child to a parent if it does not have scene node");
    assert(parentEntity.has<component::Geometry>()
           && "Can't add a child to a parent if it does not have geometry");
    assert(parentEntity.has<component::GlobalPose>()
           && "Can't add a child to a parent if it does not have global pose");
    ent::Entity newChild = *aHandle.get(graphPhase);
    assert(newChild.has<component::SceneNode>()
           && "Can't add a entity to the scene graph if it does not have a "
              "scene node");
    assert(newChild.has<component::Geometry>()
           && "Can't add a entity to the scene graph if it does not have a "
              "geometry");
    assert(newChild.has<component::GlobalPose>()
           && "Can't add a entity to the scene graph if it does not have a "
              "global position");
    component::SceneNode & parentNode =
        parentEntity.get<component::SceneNode>();
    component::SceneNode & newNode = newChild.get<component::SceneNode>();
    assert(newNode.mParent == std::nullopt
           && "Can't add a node into the scene graph that is already part of "
              "the scene graph");

    if (parentNode.mFirstChild)
    {
        EntHandle oldHandle = *parentNode.mFirstChild;
        component::SceneNode & oldNode =
            oldHandle.get(graphPhase)->get<component::SceneNode>();
        newNode.mNextChild = oldHandle;
        oldNode.mPrevChild = aHandle;
    }

    newNode.mParent = aParent;

    parentNode.mFirstChild = aHandle;
}

void removeEntityFromScene(ent::Handle<ent::Entity> aHandle)
{
    ent::Phase graphPhase;
    Entity removedChild = *aHandle.get(graphPhase);
    component::SceneNode & removedNode =
        removedChild.get<component::SceneNode>();
    assert(removedNode.mParent && "removed node does not have a parent");
    Entity parentEntity = *removedNode.mParent->get(graphPhase);
    component::SceneNode & parentNode =
        parentEntity.get<component::SceneNode>();

    if (parentNode.mFirstChild == aHandle)
    {
        parentNode.mFirstChild = removedNode.mNextChild;
    }

    if (removedNode.mNextChild)
    {
        removedNode.mNextChild->get(graphPhase)
            ->get<component::SceneNode>()
            .mPrevChild = removedNode.mPrevChild;
    }

    if (removedNode.mPrevChild)
    {
        removedNode.mPrevChild->get(graphPhase)
            ->get<component::SceneNode>()
            .mNextChild = removedNode.mNextChild;
    }

    removedNode.mParent = std::nullopt;
    removedNode.mNextChild = std::nullopt;
    removedNode.mPrevChild = std::nullopt;
}

void transferEntity(EntHandle aHandle, EntHandle aNewParent)
{
    Phase transfer;
    Entity parent = *aNewParent.get(transfer);
    assert(parent.has<component::SceneNode>()
           && "Can't add a child to a parent if it does not have scene node");
    assert(parent.has<component::Geometry>()
           && "Can't add a child to a parent if it does not have geometry");
    assert(parent.has<component::GlobalPose>()
           && "Can't add a child to a parent if it does not have global pose");
    Entity child = *aHandle.get(transfer);
    assert(child.has<component::SceneNode>()
           && "Can't add a entity to the scene graph if it does not have a "
              "scene node");
    assert(child.has<component::Geometry>()
           && "Can't add a entity to the scene graph if it does not have a "
              "geometry");
    assert(child.has<component::GlobalPose>()
           && "Can't add a entity to the scene graph if it does not have a "
              "global position");

    component::SceneNode & childNode = child.get<component::SceneNode>();
    component::Geometry & childGeo = child.get<component::Geometry>();
    component::GlobalPose & childPose = child.get<component::GlobalPose>();

    component::GlobalPose & parentPose = parent.get<component::GlobalPose>();
    if (childNode.mParent)
    {
        removeEntityFromScene(aHandle);
    }

    computeNewLocalTransform(childGeo, childPose, parentPose);

    insertEntityInScene(aHandle, aNewParent);
}

} // namespace snacgame
} // namespace ad
