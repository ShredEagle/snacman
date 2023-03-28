#pragma once
#include <entity/Entity.h>

namespace ad {
namespace snacgame {
namespace component {

struct SceneNode
{
    std::optional<ent::Handle<ent::Entity>> aFirstChild;
    std::optional<ent::Handle<ent::Entity>> aNextChild;
    std::optional<ent::Handle<ent::Entity>> aPrevChild;
    std::optional<ent::Handle<ent::Entity>> aParent;
    // HACK: (franz) this is necessary because we do not have a way to
    // compare to a non initialized handle
    std::size_t mChildCount = 0;
};

}
} // namespace snacgame
} // namespace ad
