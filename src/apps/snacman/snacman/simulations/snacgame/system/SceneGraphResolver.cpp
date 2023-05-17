#include "SceneGraphResolver.h"

#include "../SceneGraph.h"
#include "../GameParameters.h"
#include "../SceneGraph.h"
#include "../typedef.h"

#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/SceneNode.h"

#include <snacman/Profiling.h>
#include <snacman/EntityUtilities.h>

#include <math/Angle.h>
#include <math/Homogeneous.h>
#include <math/Transformations.h>
#include <cmath>

namespace ad {
namespace snacgame {
namespace system {

namespace {


template <class T_pose>
TransformMatrix computeMatrix(const T_pose & aPoseComponent)
{
    ScaleMatrix localScaling = math::trans3d::scaleUniform(aPoseComponent.mScaling);
    RotationMatrix localRotate = aPoseComponent.mOrientation.toRotationMatrix();

    // We translate the child according to the instance scaling of it's parent
    // this allows for non uniform scaling of instance without
    // introducing skew in the transform matrix of the scene graph
    TransformMatrix localTranslate =
        math::trans3d::translate(aPoseComponent.mPosition.as<math::Vec>());
    return localScaling * localRotate * localTranslate;
}

TransformMatrix resolveGlobalPos(const component::Geometry & aLocalGeo,
                                 const TransformMatrix & aParentTransform)
{
    return computeMatrix(aLocalGeo) * aParentTransform;
}

void depthFirstResolve(const component::SceneNode & aSceneNode,
                       const TransformMatrix & aParentTransform)
{
    // Lots of cache miss here which is not suprising but
    // It's it this load in particular that takes 10% of the run time of depthFirstResolve
    if (aSceneNode.mFirstChild)
    {
        const component::SceneNode fakePrevNode{
            .mNextChild = *aSceneNode.mFirstChild,
        };
        const component::SceneNode * prevNode = &fakePrevNode;

        while (prevNode->mNextChild)
        {
            ent::Handle<ent::Entity> current = *prevNode->mNextChild;
            const component::SceneNode & node = 
                current.get()->get<component::SceneNode>();
            const component::Geometry & geo =
                current.get()->get<component::Geometry>();
            component::GlobalPose & gPose =
                current.get()->get<component::GlobalPose>();
            TransformMatrix wTransform =
                resolveGlobalPos(geo, aParentTransform);

            // Decomposed matrix are used for interpolation
            // in the render thread
            decomposeMatrix(wTransform, gPose);
            gPose.mColor = geo.mColor;
            gPose.mInstanceScaling = geo.mInstanceScaling;
 
            depthFirstResolve(node, wTransform);

            prevNode = &node;
        }
    }
}

} // namespace


void updateGlobalPosition(const component::SceneNode & aSceneNode)
{
    if(aSceneNode.mParent) // otherwise root node
    {
        const auto & parentGlobalPose = snac::getComponent<component::GlobalPose>(*aSceneNode.mParent);
        depthFirstResolve(
            snac::getComponent<component::SceneNode>(*aSceneNode.mParent),
            computeMatrix(parentGlobalPose));
    }
    else
    {
        depthFirstResolve(aSceneNode, gWorldCoordTo3dCoord);
    }
}


SceneGraphResolver::SceneGraphResolver(GameContext & aGameContext,
                   ent::Handle<ent::Entity> aSceneRoot) :
    mSceneRoot{aSceneRoot}, mNodes{aGameContext.mWorld}
{
    mNodes.onRemoveEntity([](ent::Handle<ent::Entity> aHandle,
                              component::SceneNode & aNode,
                              const component::Geometry &,
                              const component::GlobalPose &) {
        removeEntityFromScene(aHandle);
    });
}

void SceneGraphResolver::update()
{
    TIME_RECURRING_CLASSFUNC(Main);
    component::SceneNode & rootNode =
        mSceneRoot.get()->get<component::SceneNode>();
    depthFirstResolve(rootNode, gWorldCoordTo3dCoord);
}

} // namespace system
} // namespace snacgame
} // namespace ad
