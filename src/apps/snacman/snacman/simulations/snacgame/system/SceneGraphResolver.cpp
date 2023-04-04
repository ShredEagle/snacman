#include "SceneGraphResolver.h"

#include "math/Angle.h"
#include "math/Homogeneous.h"
#include "math/Transformations.h"

#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/SceneNode.h"
#include "../GameParameters.h"
#include "../typedef.h"

#include <cmath>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {

namespace {

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

    float w, x, y, z;
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

void
decomposeMatrix(const TransformMatrix & aDecomposableMatrix, component::GlobalPose & aGlobPose)
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

TransformMatrix resolveGlobalPos(const component::Geometry & aLocalGeo,
                                 const TransformMatrix & aParentTransform)
{
    ScaleMatrix localScaling = math::trans3d::scaleUniform(aLocalGeo.mScaling);
    RotationMatrix localRotate = aLocalGeo.mOrientation.toRotationMatrix();

    // We translate the child according to the instance scaling of it's parent
    // this allows for non uniform scaling of instance without
    // introducing skew in the transform matrix of the scene graph
    TransformMatrix localTranslate =
        math::trans3d::translate(aLocalGeo.mPosition.as<math::Vec>());
    TransformMatrix localMatrix = localScaling * localRotate * localTranslate;
    TransformMatrix globalMatrix = localMatrix * aParentTransform;

    return globalMatrix;
}

void depthFirstResolve(const component::SceneNode & aSceneNode,
                       const TransformMatrix & aParentTransform,
                       ent::Phase & aPhase)
{
    // Lots of cache miss here which is not suprising but
    // It's it this load in particular that takes 10% of the run time of depthFirstResolve
    if (aSceneNode.mFirstChild)
    {
        const component::SceneNode fakePrevNode{
            .mNextChild = *aSceneNode.mFirstChild,
        };
        const component::SceneNode * prevNode = &fakePrevNode;

        // TODO: (franz) use the optionalness of next child and prev child
        while (prevNode->mNextChild)
        {
            ent::Handle<ent::Entity> current = *prevNode->mNextChild;
            const component::SceneNode & node = 
                current.get(aPhase)->get<component::SceneNode>();
            const component::Geometry & geo =
                current.get(aPhase)->get<component::Geometry>();
            component::GlobalPose & gPose =
                current.get(aPhase)->get<component::GlobalPose>();
            TransformMatrix wTransform =
                resolveGlobalPos(geo, aParentTransform);

            // Decomposed matrix are used for interpolation
            // in the render thread
            decomposeMatrix(wTransform, gPose);
            gPose.mColor = geo.mColor;
            gPose.mInstanceScaling = geo.mInstanceScaling;
 
            depthFirstResolve(node, wTransform, aPhase);

            prevNode = &node;
        }
    }
}

} // namespace

void SceneGraphResolver::update()
{
    TIME_RECURRING_CLASSFUNC(Main);
    ent::Phase resolve;
    component::SceneNode & rootNode =
        mSceneRoot.get(resolve)->get<component::SceneNode>();
    depthFirstResolve(rootNode, gWorldCoordTo3dCoord, resolve);
}

} // namespace system
} // namespace snacgame
} // namespace ad
