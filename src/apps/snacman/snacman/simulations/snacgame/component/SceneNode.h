#pragma once

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

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

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mFirstChild);
        archive & SERIAL_PARAM(mNextChild);
        archive & SERIAL_PARAM(mPrevChild);
        archive & SERIAL_PARAM(mParent);
        archive & SERIAL_PARAM(mName);
    }
};

SNAC_SERIAL_REGISTER(SceneNode)


}
} // namespace snacgame
} // namespace ad
