#pragma once

#include "../OrbitalControlInput.h"

#include "../component/SceneNode.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>
#include <entity/Wrap.h>

#include <map>
#include <markovjunior/Grid.h>
#include <optional>
#include <string>
#include <variant>

namespace ad {

namespace snacgame {
namespace component {
struct MappingContext;
}
namespace scene {

const inline std::string gQuitTransitionName = "quit";

struct MenuSceneInfo
{};

struct JoinGameSceneInfo
{
    int mTransitionControllerId = -1;
};

struct GameSceneInfo
{};

struct DisconnectedControllerInfo
{
    std::vector<int> mDisconnectedControllerId;
};

struct Transition
{
    std::string mTransitionName;
    bool shouldTeardown = true;
    bool shouldSetup = true;
    std::variant<MenuSceneInfo, JoinGameSceneInfo, GameSceneInfo, DisconnectedControllerInfo> mSceneInfo;

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

namespace snac {
struct Time;
class Orbital;
}

struct RawInput;

namespace snacgame {

struct GameContext;

namespace scene {

class Scene
{
public:
    // TODO :(franz) make a cpp file please
    Scene(std::string aName,
          GameContext & aGameContext,
          ent::Wrap<component::MappingContext> & aContext);
    Scene(const Scene &) = default;
    Scene(Scene && aScene) noexcept;

    Scene & operator=(const Scene &) = delete;
    Scene & operator=(Scene &&) = delete;
    virtual ~Scene() = default;

    virtual void onExit(Transition aTransition) = 0;
    virtual void onEnter(Transition aTransition) = 0;
    virtual void update(const snac::Time & aTime, RawInput & aInput) = 0;

    std::string mName;

protected:
    GameContext & mGameContext;
    ent::Handle<ent::Entity> mSystems;
    std::vector<ent::Handle<ent::Entity>> mOwnedEntities;
    ent::Wrap<component::MappingContext> & mMappingContext;
    ent::Query<OrbitalControlInput> mCameraQuery;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
