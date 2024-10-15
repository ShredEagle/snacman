#pragma once

#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

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

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mFirstChild));
        aWitness.witness(NVP(mNextChild));
        aWitness.witness(NVP(mPrevChild));
        aWitness.witness(NVP(mParent));
        aWitness.witness(NVP(mName));
    }
};

REFLEXION_REGISTER(SceneNode)

}
} // namespace snacgame
} // namespace ad
