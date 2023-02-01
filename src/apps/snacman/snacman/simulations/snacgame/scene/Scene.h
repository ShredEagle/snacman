#pragma once

#include "markovjunior/Grid.h"
#include "snacman/Input.h"
#include "snacman/simulations/snacgame/EntityWrap.h"

#include "../component/Context.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <map>
#include <optional>
#include <string>

namespace ad {
namespace snacgame {
namespace scene {

const inline std::string gQuitTransitionName = "quit";

struct Transition
{
    std::string mTransitionName;
    bool shouldTeardown = true;
    bool shouldSetup = true;

    bool operator==(const Transition & aRhs) const
    {
        return mTransitionName == aRhs.mTransitionName;
    }
};

} // namespace scene
} // namespace snacgame
} // namespace ad

namespace std {
template <>
struct hash<ad::snacgame::scene::Transition>
{
    size_t operator()(const ad::snacgame::scene::Transition & aTransition) const
    {
        return hash<std::string>()(aTransition.mTransitionName);
    }
};
}; // namespace std

namespace ad {
namespace snacgame {
namespace scene {

struct SceneId
{
    std::size_t mIndexInPossibleState;
};
class Scene
{
public:
    Scene(const std::string & aName,
          ent::EntityManager & aWorld,
          EntityWrap<component::MappingContext> & aContext) :
        mName{aName},
        mWorld{aWorld},
        mSystems{mWorld.addEntity()},
        mContext{aContext}
    {}
    Scene(const Scene &) = default;
    Scene(Scene &&) = delete;
    Scene & operator=(const Scene &) = delete;
    Scene & operator=(Scene &&) = delete;
    virtual ~Scene() = default;

    virtual std::optional<Transition> update(float aDelta,
                                             RawInput & aInput) = 0;
    virtual void setup(const Transition & aTransition, RawInput & aInput) = 0;
    virtual void teardown(RawInput & aInput) = 0;

    std::string mName;
    std::unordered_map<Transition, SceneId> mStateTransition;

protected:
    ent::EntityManager & mWorld;
    ent::Handle<ent::Entity> mSystems;
    std::vector<ent::Handle<ent::Entity>> mOwnedEntities;
    EntityWrap<component::MappingContext> & mContext;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
