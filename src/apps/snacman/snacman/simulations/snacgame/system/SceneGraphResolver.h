#pragma once


#include "snacman/simulations/snacgame/GameContext.h"
#include <snacman/Logging.h>
#include "../SceneGraph.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {

namespace component {
struct GlobalPose;
struct SceneNode;
struct Geometry;
}

namespace system {
class SceneGraphResolver
{
public:
    SceneGraphResolver(GameContext & aGameContext,
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

    void update();

private:
    ent::Handle<ent::Entity> mSceneRoot;
    ent::Query<component::SceneNode,
               component::Geometry,
               component::GlobalPose>
        mNodes;
};
} // namespace system
} // namespace snacgame
} // namespace ad
