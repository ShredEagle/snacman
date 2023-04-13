#pragma once
#include <entity/Entity.h>

namespace ad {
namespace snacgame {
namespace component {

struct SceneNode
{
    std::optional<ent::Handle<ent::Entity>> mFirstChild;
    std::optional<ent::Handle<ent::Entity>> mNextChild;
    std::optional<ent::Handle<ent::Entity>> mPrevChild;
    std::optional<ent::Handle<ent::Entity>> mParent;
};

}
} // namespace snacgame
} // namespace ad
