#pragma once
#include <entity/Entity.h>

namespace ad {
namespace snacgame {
namespace component {

struct SceneNode
{
    ent::Handle<ent::Entity> aFirstChild;
    ent::Handle<ent::Entity> aNextChild;
    ent::Handle<ent::Entity> aPrevChild;
    ent::Handle<ent::Entity> aParent;
};

}
} // namespace snacgame
} // namespace ad
