// Splitted between two-files because there was an error on Windows:
// fatal  error C1128: number of sections exceeded object file format limit: compile with /bigobj

#include "GameScene.h"

#include "../system/AllowMovement.h"
#include "../system/ConsolidateGridMovement.h"
#include "../system/Debug_BoundingBoxes.h"
#include "../system/EatPill.h"
#include "../system/IntegratePlayerMovement.h"
#include "../system/LevelCreator.h"
#include "../system/MovementIntegration.h"
#include "../system/Pathfinding.h"
#include "../system/PlayerInvulFrame.h"
#include "../system/PlayerSpawner.h"
#include "../system/PortalManagement.h"
#include "../system/PowerUpUsage.h"
#include "../system/RoundMonitor.h"
#include "../system/SceneGraphResolver.h"

#include "../typedef.h"


namespace ad {
namespace snacgame {
namespace scene {


namespace {

    void setupLevel(GameContext & aGameContext, Phase & aPhase)
    {
        aGameContext.mLevel->get(aPhase)->add(component::LevelToCreate{});
        aGameContext.mLevel->get(aPhase)->add(component::SceneNode{});
        aGameContext.mLevel->get(aPhase)->add(
            component::Geometry{.mPosition = {-7.f, -7.f, 0.f}});
        aGameContext.mLevel->get(aPhase)->add(component::GlobalPose{});
    }

} // unnamed namespace


void GameScene::setup(const Transition & aTransition, RawInput & aInput)
{
    {
        Phase init;
        setupLevel(mGameContext, init);
        mSystems.get(init)
            ->add(system::SceneGraphResolver{mGameContext, mSceneRoot})
            .add(system::PlayerSpawner{mGameContext})
            .add(system::RoundMonitor{mGameContext})
            .add(system::PlayerInvulFrame{mGameContext})
            .add(system::AllowMovement{mGameContext})
            .add(system::ConsolidateGridMovement{mGameContext})
            .add(system::IntegratePlayerMovement{mGameContext})
            .add(system::LevelCreator{mGameContext})
            .add(system::MovementIntegration{mGameContext})
            .add(system::EatPill{mGameContext})
            .add(system::PortalManagement{mGameContext})
            .add(system::PowerUpUsage{mGameContext})
            .add(system::Pathfinding{mGameContext})
            .add(system::Debug_BoundingBoxes{mGameContext})
            ;
    }

    // Can't insert mLevel before the createLevel phase is over
    // otherwise mLevel does not have the correct component
    insertEntityInScene(*mGameContext.mLevel, mSceneRoot);
}


} // namespace scene
} // namespace snacgame
} // namespace ad
