#pragma once

#include "Scene.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

#include <string>

namespace ad {

namespace snac {
struct Time;
}

struct RawInput;

namespace snacgame {

struct GameContext;
template <class T_wrapped> struct EntityWrap;

namespace component {
struct MappingContext;
struct PlayerSlot;
struct PlayerHud;
struct PlayerRoundData;
struct Controller;
struct PathToOnGrid;
struct LevelTile;
struct RoundTransient;
struct LevelSetupData;
}

namespace scene {

class GameScene : public Scene
{
public:
    GameScene(std::string aName,
            GameContext & aGameContext,
              EntityWrap<component::MappingContext> & aContext,
              ent::Handle<ent::Entity> aSceneRoot
              );

    std::optional<Transition> update(const snac::Time & aTime,
                                     RawInput & aInput) override;

    void setup(const Transition & aTransition, RawInput & aInput) override;

    void teardown(RawInput & aInput) override;

private:
    ent::Query<component::LevelTile> mTiles;
    ent::Query<component::RoundTransient> mRoundTransients;
    ent::Query<component::PlayerSlot> mSlots;
    ent::Query<component::PlayerHud> mHuds;
    ent::Query<component::PlayerRoundData, component::Controller> mPlayers;
    ent::Query<component::PathToOnGrid> mPathfinders;

    std::optional<ent::Handle<ent::Entity>> mStageDecor;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
