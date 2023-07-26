#pragma once

#include "Scene.h"

#include <snac-renderer/Camera.h>

#include <entity/EntityManager.h>
#include <entity/Query.h>
#include <entity/Wrap.h>
#include <string>

namespace ad {

namespace snac {
struct Time;
}

struct RawInput;

namespace snacgame {

struct GameContext;
template <class T_wrapped>
struct EntityWrap;

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
} // namespace component

namespace scene {

class GameScene : public Scene
{
public:
    GameScene(GameContext & aGameContext,
              ent::Wrap<component::MappingContext> & aContext);

    void update(const snac::Time & aTime, RawInput & aInput) override;

    void onEnter(Transition aTransition) override;

    void onExit(Transition aTransition) override;

    ent::Wrap<component::LevelSetupData> mLevelData;

    static constexpr char sFromPauseTransition[] = "GameFromPauseTransition";
    static constexpr char sToPauseTransition[] = "Pause";
private:
    ent::Query<component::LevelTile> mTiles;
    ent::Query<component::RoundTransient> mRoundTransients;
    ent::Query<component::PlayerSlot> mSlots;
    ent::Query<component::PlayerHud> mHuds;
    ent::Query<component::PlayerRoundData, component::Controller> mPlayers;
    ent::Query<component::PathToOnGrid> mPathfinders;
    ent::Handle<ent::Entity> mLevel;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
