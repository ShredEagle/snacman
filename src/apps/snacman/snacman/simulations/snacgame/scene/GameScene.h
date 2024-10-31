#pragma once

#include "Scene.h"
#include "snacman/Timing.h"
#include "snacman/simulations/snacgame/component/Geometry.h"
#include "snacman/simulations/snacgame/component/Speed.h"

#include <snac-renderer-V1/Camera.h>

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
struct Gravity;
} // namespace component

namespace scene {

enum class GamePhase
{
    SpawningSequence,
    Playing,
    VictorySequence,
};

struct GameSceneData
{
    GamePhase mPhase = GamePhase::SpawningSequence;
    snac::Clock::time_point mStartOfScene = snac::Clock::now(); 
};

class GameScene : public Scene
{
public:
    GameScene(GameContext & aGameContext,
              ent::Wrap<component::MappingContext> & aContext);

    void update(const snac::Time & aTime, RawInput & aInput) override;

    void onEnter(Transition aTransition) override;

    void onExit(Transition aTransition) override;

    ent::Wrap<component::LevelSetupData> mLevelData;
    ent::Wrap<GameSceneData> mSceneData;

    static constexpr char sFromPauseTransition[] = "GameFromPauseTransition";
    static constexpr char sToPauseTransition[] = "Pause";
    static constexpr char sToDisconnectedControllerTransition[] = "DisconnectedController";
private:

    ent::Handle<ent::Entity> createSpawningPhaseText(const std::string & aText, const snac::Time & aTime);

    ent::Query<component::LevelTile> mTiles;
    ent::Query<component::RoundTransient> mRoundTransients;
    ent::Query<component::PlayerSlot> mSlots;
    ent::Query<component::PlayerHud> mHuds;
    ent::Query<component::Geometry, component::PlayerRoundData, component::Controller> mPlayers;
    ent::Query<component::Gravity, component::Geometry> mFallingPlayers;
    ent::Query<component::PathToOnGrid> mPathfinders;
    ent::Handle<ent::Entity> mLevel;
    ent::Handle<ent::Entity> mReadyGoText;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
