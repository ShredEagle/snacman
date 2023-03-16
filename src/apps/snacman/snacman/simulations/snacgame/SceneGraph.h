#pragma once

#include "component/Geometry.h"
#include "component/GlobalPosition.h"
#include "component/SceneNode.h"
#include "component/SceneRoot.h"

namespace ad {
namespace snac {
void makeEntityRoot(ent::Handle<ent::Entity> & aHandle)
{
    ent::Phase rootPhase;
    aHandle.get(rootPhase)->add(snacgame::component::SceneRoot{})
        .add(snacgame::component::Geometry{})
        .add(snacgame::component::GlobalPosition{});
}

void addEntityToSceneGraph(ent::Handle<ent::Entity> & aHandle, ent::Handle<ent::Entity> & aParent)
{
    ent::Phase graphPhase;
    ent::Entity parentEntity = *aParent.get(graphPhase);
    ent::Entity newChild = *aHandle.get(graphPhase);
    if (parentEntity.has<snacgame::component::SceneRoot>())
    {
        if (parentEntity.get<snacgame::component::SceneRoot>().aFirstChild)
        {
            ent::Handle<ent::Entity> oldHandle = *parentEntity.get<snacgame::component::SceneRoot>().aFirstChild;
            snacgame::component::SceneNode & newNode = newChild.get<snacgame::component::SceneNode>();
            snacgame::component::SceneNode & oldNode = oldHandle.get(graphPhase)->get<snacgame::component::SceneNode>();
            newNode.aNextChild = oldHandle;
            newNode.aPrevChild = aParent;
            oldNode.aPrevChild = aHandle;
        }
        else
        {
            newChild.get<snacgame::component::SceneNode>().aNextChild = aParent;
            newChild.get<snacgame::component::SceneNode>().aPrevChild = aParent;
        }
        parentEntity.get<snacgame::component::SceneRoot>().aFirstChild = aHandle;
    }
    else if (parentEntity.has<snacgame::component::SceneNode>())
    {
    }
}
}
}
