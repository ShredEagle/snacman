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
    float fourXSquaredMinus1 = aMatrix.at(0, 0) - aMatrix.at(1, 1) - aMatrix.at(2, 2);
    float fourYSquaredMinus1 = aMatrix.at(1, 1) - aMatrix.at(2, 2) - aMatrix.at(0, 0);
    float fourZSquaredMinus1 = aMatrix.at(2, 2) - aMatrix.at(0, 0) - aMatrix.at(1, 1);

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

    float biggestVal = std::sqrt(fourBiggestSquared + 1.f) / 2;
    float mult = 1 / (4 * biggestVal);

    float w, x, y, z;
    switch (biggestIndex)
    {
        case 0:
            w = biggestVal;
            x = (aMatrix.at(1, 2) - aMatrix.at(2, 1)) * mult;
            y = (aMatrix.at(2, 0) - aMatrix.at(0, 2)) * mult;
            z = (aMatrix.at(0, 1) - aMatrix.at(1, 0)) * mult;
            break;
        case 1:
            x = biggestVal;
            w = (aMatrix.at(1, 2) - aMatrix.at(2, 1)) * mult;
            y = (aMatrix.at(0, 1) + aMatrix.at(1, 0)) * mult;
            z = (aMatrix.at(2, 0) + aMatrix.at(0, 2)) * mult;
            break;
        case 2:
            y = biggestVal;
            w = (aMatrix.at(2, 0) - aMatrix.at(0, 2)) * mult;
            x = (aMatrix.at(0, 1) + aMatrix.at(1, 0)) * mult;
            z = (aMatrix.at(1, 2) + aMatrix.at(2, 1)) * mult;
            break;
        case 3:
            z = biggestVal;
            w = (aMatrix.at(0, 1) - aMatrix.at(1, 0)) * mult;
            y = (aMatrix.at(1, 2) + aMatrix.at(2, 1)) * mult;
            x = (aMatrix.at(2, 0) + aMatrix.at(0, 2)) * mult;
            break;
    }
    return Quat_f(x, y, z, w);
}

component::GlobalPose
decomposeMatrix(const TransformMatrix & aDecomposableMatrix)
{
    // We only use uniform scaling for our heritable transform matrix
    // So no skew in here
    assert(aDecomposableMatrix.at(0, 3) == 0.f && "Can't decompose a transform matrix with skew");

    component::GlobalPose gPose;
    gPose.mPosition = getTranslation(aDecomposableMatrix);
    gPose.mScaling = getUniformScale(aDecomposableMatrix);
    gPose.mOrientation = getRotation(aDecomposableMatrix, gPose.mScaling);
    return gPose;
}

TransformMatrix resolveGlobalPos(const component::Geometry & aLocalGeo,
                                 const TransformMatrix & aParentTransform)
{
    ScaleMatrix localScaling = math::trans3d::scale(
        aLocalGeo.mScaling, aLocalGeo.mScaling, aLocalGeo.mScaling);
    TransformMatrix localRotate = aLocalGeo.mOrientation.toRotationMatrix();

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
    if (aSceneNode.aFirstChild)
    {
        ent::Handle<ent::Entity> current = *aSceneNode.aFirstChild;

        for (int i = 0; i < aSceneNode.mChildCount; i++)
        {
            const component::SceneNode & node =
                current.get(aPhase)->get<component::SceneNode>();
            const component::Geometry geo =
                current.get(aPhase)->get<component::Geometry>();
            component::GlobalPose & gPose =
                current.get(aPhase)->get<component::GlobalPose>();
            TransformMatrix wTransform =
                resolveGlobalPos(geo, aParentTransform);

            // Decomposed matrix are used for interpolation
            // in the render thread
            gPose = decomposeMatrix(wTransform);
            gPose.mColor = geo.mColor;
            gPose.mInstanceScaling = geo.mInstanceScaling;
 
            depthFirstResolve(node, wTransform, aPhase);
            current = *node.aNextChild;
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
