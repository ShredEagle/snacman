#pragma once


#include "../GameContext.h"
#include "../SceneGraph.h"

#include <snacman/Logging.h>

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
                       ent::Handle<ent::Entity> aSceneRoot);

    void update();

private:
    ent::Handle<ent::Entity> mSceneRoot;
    ent::Query<component::SceneNode,
               component::Geometry,
               component::GlobalPose>
        mNodes;
};


void updateGlobalPosition(const component::SceneNode & aSceneNode);


} // namespace system
} // namespace snacgame
} // namespace ad
