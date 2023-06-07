#pragma once

#include "../EntityWrap.h"
#include "../GameContext.h"

#include "../component/SceneNode.h"

#include <snacman/Timing.h>

#include <markovjunior/Grid.h>

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <map>
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
    int mTransitionControllerId = -1;
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
    //TODO :(franz) make a cpp file please
    Scene(std::string aName,
          GameContext & aGameContext,
          EntityWrap<component::MappingContext> & aContext,
          ent::Handle<ent::Entity> aSceneRoot) :
        mName{aName},
        mSceneRoot{aSceneRoot},
        mGameContext{aGameContext},
        mSystems{mGameContext.mWorld.addEntity()},
        mContext{aContext}
    {}
    Scene(const Scene &) = default;
    Scene(Scene &&) = delete;
    Scene & operator=(const Scene &) = delete;
    Scene & operator=(Scene &&) = delete;
    virtual ~Scene() = default;

    virtual std::optional<Transition> update(const snac::Time & aTime,
                                             RawInput & aInput) = 0;

    virtual void setup(const Transition & aTransition, RawInput & aInput) = 0;

    virtual void teardown(RawInput & aInput) = 0;

    std::string mName;
    std::unordered_map<Transition, SceneId> mStateTransition;
    ent::Handle<ent::Entity> mSceneRoot;

protected:
    GameContext & mGameContext;
    ent::Handle<ent::Entity> mSystems;
    std::vector<ent::Handle<ent::Entity>> mOwnedEntities;
    EntityWrap<component::MappingContext> & mContext;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
