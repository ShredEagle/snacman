#pragma once

#include "snacman/simulations/snacgame/component/SceneNode.h"
#include "../EntityWrap.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <map>
#include <markovjunior/Grid.h>
#include <optional>
#include <string>

namespace ad {

struct RawInput;

namespace snacgame {
    namespace component { struct MappingContext; }
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

struct GameContext;

namespace scene {


struct SceneId
{
    std::size_t mIndexInPossibleState;
};


class Scene
{
public:
    //TODO :(franz) make a cpp file please
    Scene(const std::string & aName,
          ent::EntityManager & aWorld,
          EntityWrap<component::MappingContext> & aContext,
          ent::Handle<ent::Entity> aSceneRoot) :
        mName{aName},
        mSceneRoot{aSceneRoot},
        mWorld{aWorld},
        mSystems{mWorld.addEntity()},
        mContext{aContext}
    {}
    Scene(const Scene &) = default;
    Scene(Scene &&) = delete;
    Scene & operator=(const Scene &) = delete;
    Scene & operator=(Scene &&) = delete;
    virtual ~Scene() = default;

    virtual std::optional<Transition> update(GameContext & aContext,
                                             float aDelta,
                                             RawInput & aInput) = 0;

    virtual void setup(GameContext & aContext, const Transition & aTransition, RawInput & aInput) = 0;

    virtual void teardown(GameContext & aContext, RawInput & aInput) = 0;

    std::string mName;
    std::unordered_map<Transition, SceneId> mStateTransition;
    ent::Handle<ent::Entity> mSceneRoot;

protected:
    ent::EntityManager & mWorld;
    ent::Handle<ent::Entity> mSystems;
    std::vector<ent::Handle<ent::Entity>> mOwnedEntities;
    EntityWrap<component::MappingContext> & mContext;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
