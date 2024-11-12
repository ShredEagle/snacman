#pragma once

#include "Scene.h"
#include "snacman/Timing.h"
#include "snacman/simulations/snacgame/component/AllowedMovement.h"
#include "snacman/simulations/snacgame/component/Geometry.h"
#include "snacman/simulations/snacgame/component/Speed.h"
#include "snacman/simulations/snacgame/component/Tags.h"

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
    NullPhase,
};

struct GameSceneData
{
    GamePhase mPhase = GamePhase::NullPhase;
    snac::Clock::time_point mStartOfPhase; 

    inline void changePhase(const GamePhase & aPhase, const snac::Time & aTime)
    {
        mPhase = aPhase;
        mStartOfPhase = aTime.mTimepoint;
    }
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

    char findWinner();
    ent::Handle<ent::Entity> createSpawningPhaseText(const std::string & aText, const math::hdr::Rgba_f & aColor, const snac::Time & aTimer);

    ent::Query<component::LevelTile> mTiles;
    ent::Query<component::RoundTransient> mRoundTransients;
    ent::Query<component::PlayerSlot> mSlots;
    ent::Query<component::PlayerHud> mHuds;
    ent::Query<component::Geometry, component::PlayerRoundData, component::Controller, component::AllowedMovement> mPlayers;
    ent::Query<component::PathToOnGrid> mPathfinders;
    ent::Query<component::Crown> mCrowns;
    ent::Handle<ent::Entity> mLevel;
    ent::Handle<ent::Entity> mReadyText;
    ent::Handle<ent::Entity> mGoText;
    ent::Handle<ent::Entity> mVictoryText;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
