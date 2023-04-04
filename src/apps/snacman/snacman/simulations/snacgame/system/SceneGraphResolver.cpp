#include "SceneGraphResolver.h"

#include "math/Angle.h"
#include "math/Homogeneous.h"
#include "math/Transformations.h"
#include "snacman/simulations/snacgame/SceneGraph.h"

#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/SceneNode.h"
#include "../GameParameters.h"
#include "../SceneGraph.h"
#include "../typedef.h"

#include <cmath>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {

namespace {
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
