// Splitted between two-files because there was an error on Windows:
// fatal  error C1128: number of sections exceeded object file format limit:
// compile with /bigobj

#include "GameScene.h"
#include "snacman/Input.h"

#include "../component/AllowedMovement.h"
#include "../component/Collision.h"
#include "../component/Controller.h"
#include "../component/Explosion.h"
#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/MovementScreenSpace.h"
#include "../component/PathToOnGrid.h"
#include "../component/PlayerHud.h"
#include "../component/PlayerGameData.h"
#include "../component/PlayerSlot.h"
#include "../component/PlayerRoundData.h"
#include "../component/Portal.h"
#include "../component/PoseScreenSpace.h"
#include "../component/PowerUp.h"
#include "../component/RigAnimation.h"
#include "../component/SceneNode.h"
#include "../component/Spawner.h"
#include "../component/Speed.h"
#include "../component/Tags.h"
#include "../component/VisualModel.h"
#include "../GameContext.h"
#include "../Entities.h"
#include "../SceneGraph.h"
#include "../system/AdvanceAnimations.h"
#include "../system/AllowMovement.h"
#include "../system/AnimationManager.h"
#include "../system/ConsolidateGridMovement.h"
#include "../system/Debug_BoundingBoxes.h"
#include "../system/EatPill.h"
#include "../system/Explosion.h"
#include "../system/IntegratePlayerMovement.h"
#include "../system/LevelManager.h"
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

void GameScene::setup(const Transition & aTransition, RawInput & aInput)
{
    // Preload models to avoid loading time when they first appear in the game
    mGameContext.mResources.getModel("models/collar/collar.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/teleport/teleport.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/missile/missile.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/boom/boom.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/portal/portal.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/dog/dog.gltf",
                                     "effects/MeshTextures.sefx");
    mGameContext.mResources.getModel("models/missile/area.gltf",
                                     "effects/MeshTextures.sefx");
    {
        Phase init;
        mSystems.get(init)
            ->add(system::SceneGraphResolver{mGameContext, mSceneRoot})
            .add(system::PlayerSpawner{mGameContext})
            .add(system::RoundMonitor{mGameContext})
            .add(system::PlayerInvulFrame{mGameContext})
            .add(system::Explosion{mGameContext})
            .add(system::AllowMovement{mGameContext})
            .add(system::ConsolidateGridMovement{mGameContext})
            .add(system::IntegratePlayerMovement{mGameContext})
            .add(system::LevelManager{mGameContext})
            .add(system::MovementIntegration{mGameContext})
            .add(system::AdvanceAnimations{mGameContext})
            .add(system::AnimationManager{mGameContext})
            .add(system::EatPill{mGameContext})
            .add(system::PortalManagement{mGameContext})
            .add(system::PowerUpUsage{mGameContext})
            .add(system::Pathfinding{mGameContext})
            .add(system::Debug_BoundingBoxes{mGameContext});
    }

    // Can't insert mLevel before the createLevel phase is over
    // otherwise mLevel does not have the correct component

    mStageDecor = createStageDecor(mGameContext);
    insertEntityInScene(*mStageDecor, mSceneRoot);

    addPlayer(mGameContext, aTransition.mTransitionControllerId);
}

} // namespace scene
} // namespace snacgame
} // namespace ad
