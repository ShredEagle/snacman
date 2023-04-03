#include "SceneGraph.h"

#include "typedef.h"

#include "component/Geometry.h"
#include "component/GlobalPose.h"
#include "component/SceneNode.h"

#include <entity/EntityManager.h>

namespace ad {
namespace snacgame {

//Allows to easily insert a node with a transformation into the scene graph
EntHandle insertTransformNode(ent::EntityManager & aWorld, component::Geometry aGeometry, ent::Handle<ent::Entity> aParent)
{
    EntHandle nodeEnt = aWorld.addEntity();

    {
        ent::Phase graphPhase;

        nodeEnt.get(graphPhase)->add(aGeometry)
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
    assert(parentEntity.has<component::SceneNode>()
           && "Can't add a child to a parent if it does not have scene node");
    assert(parentEntity.has<component::Geometry>()
           && "Can't add a child to a parent if it does not have geometry");
    assert(parentEntity.has<component::GlobalPose>()
           && "Can't add a child to a parent if it does not have global pose");
    ent::Entity newChild = *aHandle.get(graphPhase);
    assert(newChild.has<component::SceneNode>()
           && "Can't add a entity to the scene graph if it does not have a scene node");
    assert(newChild.has<component::Geometry>()
           && "Can't add a entity to the scene graph if it does not have a geometry");
    assert(newChild.has<component::GlobalPose>()
           && "Can't add a entity to the scene graph if it does not have a global position");
    component::SceneNode & parentNode =
        parentEntity.get<component::SceneNode>();
    component::SceneNode & newNode = newChild.get<component::SceneNode>();

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
    component::SceneNode & removedNode = removedChild.get<component::SceneNode>();
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
        removedNode.mNextChild->get(graphPhase)->get<component::SceneNode>().mPrevChild = removedNode.mPrevChild;
    }

    if (removedNode.mPrevChild)
    {
        removedNode.mPrevChild->get(graphPhase)->get<component::SceneNode>().mNextChild = removedNode.mNextChild;
    }

    removedNode.mParent = std::nullopt;
    removedNode.mNextChild = std::nullopt;
    removedNode.mPrevChild = std::nullopt;
    removedNode.mFirstChild = std::nullopt;
}
} // namespace snacgame
} // namespace ad
