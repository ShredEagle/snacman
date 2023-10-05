#pragma once
#include <entity/Entity.h>

namespace ad {
namespace snacgame {
namespace component {

struct SceneNode
{
    bool hasChild() const
    { return mFirstChild.isValid(); }

    ent::Handle<ent::Entity> mFirstChild;
    ent::Handle<ent::Entity> mNextChild;
    ent::Handle<ent::Entity> mPrevChild;
    ent::Handle<ent::Entity> mParent;
};


}
} // namespace snacgame
} // namespace ad
