#pragma once
#include <entity/Entity.h>

namespace ad {
namespace snacgame {
namespace component {

struct SceneNode
{
    bool hasChild() const
    { return mFirstChild.has_value(); }

    bool hasName() const
    { return !mName.empty(); }

    std::optional<ent::Handle<ent::Entity>> mFirstChild;
    std::optional<ent::Handle<ent::Entity>> mNextChild;
    std::optional<ent::Handle<ent::Entity>> mPrevChild;
    std::optional<ent::Handle<ent::Entity>> mParent;
    // TODO There should not be a name here directly, but this makes debugging easier
    std::string mName;
};


}
} // namespace snacgame
} // namespace ad
