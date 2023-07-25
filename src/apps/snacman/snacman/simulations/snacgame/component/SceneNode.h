#pragma once
#include <entity/Entity.h>

namespace ad {
namespace snacgame {
namespace component {

struct SceneNode
{
    bool hasChild() const
    { return mFirstChild.isValid(); }

    bool hasName() const
    { return !mName.empty(); }

    ent::Handle<ent::Entity> mFirstChild;
    ent::Handle<ent::Entity> mNextChild;
    ent::Handle<ent::Entity> mPrevChild;
    ent::Handle<ent::Entity> mParent;
    // TODO There should not be a name here directly, but this makes debugging easier
    std::string mName;
};


}
} // namespace snacgame
} // namespace ad
