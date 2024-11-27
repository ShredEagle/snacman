#include "JoinGameScene.h"

#include "../ModelInfos.h"
#include "../typedef.h"
#include "../GameContext.h"

#include "../component/PlayerSlot.h"
#include "../component/PlayerGameData.h"
#include "../component/PlayerHud.h"
#include "../component/PathToOnGrid.h"
#include "../component/MenuItem.h"
#include "../component/Text.h"
#include "../component/Tags.h"
#include "../component/GlobalPose.h"
#include "../component/Geometry.h"
#include "../component/SceneNode.h"
#include "../component/Controller.h"
#include "../component/PlayerRoundData.h"
#include "../component/RigAnimation.h"
#include "../system/SceneStack.h"

#include "snacman/EntityUtilities.h"
#include "snacman/simulations/snacgame/Entities.h"
#include "snacman/simulations/snacgame/GameParameters.h"
#include "snacman/simulations/snacgame/InputCommandConverter.h"
#include "snacman/simulations/snacgame/SceneGraph.h"
#include "snacman/simulations/snacgame/component/PlayerJoinData.h"
#include "snacman/simulations/snacgame/scene/GameScene.h"
#include "snacman/simulations/snacgame/scene/MenuScene.h"
#include "snacman/simulations/snacgame/scene/Scene.h"
#include "snacman/simulations/snacgame/system/AdvanceAnimations.h"
#include "snacman/simulations/snacgame/system/InputProcessor.h"
#include "snacman/simulations/snacgame/system/SceneGraphResolver.h"

#include <snacman/QueryManipulation.h>

namespace ad {
namespace snacgame {
namespace scene {

constexpr math::Spherical<float> gJoinGameSceneInitialCameraSpherical{
    13.5f,
    math::Radian<float>{0.956f},
    math::Turn<float>{0.f}
};

constexpr math::Position<3, float> gJoinGameSceneInitialCameraPos{
    0.f, -2.237f, -8.f
};

JoinGameScene::JoinGameScene(GameContext & aGameContext,
                             ent::Wrap<component::MappingContext> & aMappingContext) :
    Scene(gJoinGameSceneName, aGameContext, aMappingContext),
    mSlots{aGameContext.mWorld},
    mJoinGameRoot{mGameContext.mWorld.addEntity("join game root")}
{
    {
        Phase createRoot;
        Entity root = *mJoinGameRoot.get(createRoot);
        addGeoNode(aGameContext, root);
    }

    insertEntityInScene(mJoinGameRoot, mGameContext.mSceneRoot);

    mGameContext.mResources.getModel(gSlotNumbers[0].mPath,
                                     gSlotNumbers[0].mProgPath);
    mGameContext.mResources.getModel(gSlotNumbers[1].mPath,
                                     gSlotNumbers[1].mProgPath);
    mGameContext.mResources.getModel(gSlotNumbers[2].mPath,
                                     gSlotNumbers[2].mProgPath);
    mGameContext.mResources.getModel(gSlotNumbers[3].mPath,
                                     gSlotNumbers[3].mProgPath);

    EntHandle camera = snac::getFirstHandle(mCameraQuery);
    renderer::Orbital & camOrbital = snac::getComponent<system::OrbitalCamera>(camera).mControl.mOrbital;
    camOrbital.mSpherical = gJoinGameSceneInitialCameraSpherical;
    camOrbital.mSphericalOrigin = gJoinGameSceneInitialCameraPos;
}

void JoinGameScene::update(const snac::Time & aTime, RawInput & aInput)
{
    Phase update;

    std::vector<ControllerCommand> controllerCommands =
        mSystems.get()
            ->get<system::InputProcessor>()
            .mapControllersInput(aInput, "player", "unbound");

    std::vector<ControllerCommand> quittingOrDisconnectedPlayers;
    bool start = false;

    for (auto controller : controllerCommands)
    {
        if (controller.mBound)
        {
            if (controller.mConnected)
            {
                if (controller.mInput.mCommand == gQuitCommand)
                {
                    quittingOrDisconnectedPlayers.push_back(controller);
                }
                start |= controller.mInput.mCommand == gPlayerSelect;
            }
            else
            {
                quittingOrDisconnectedPlayers.push_back(controller);
            }
        }
        else
        {
            if (controller.mInput.mCommand & gJoin)
            {
                addPlayerFromControllerId(controller.mId);
            }
        }
    }

    for (auto command : quittingOrDisconnectedPlayers)
    {
        Phase destroyQuitter;
        EntHandle quiter = snac::getFirstHandle(mSlots, [&command](component::Controller & aController) { return (int)aController.mControllerId == command.mId;});
        EntHandle model = snac::getComponent<component::PlayerJoinData>(quiter).mJoinPlayerModel;
        quiter.get(destroyQuitter)->erase();
        eraseEntityRecursive(model, destroyQuitter);
    }

    if (mSlots.countMatches() == 0)
    {
        mGameContext.mSceneStack->popScene(Transition{.mTransitionType = TransType::QuitToMenu});
        mGameContext.mSceneStack->pushScene(
            std::make_shared<MenuScene>(mGameContext, mMappingContext));
        return;
    }

    if (start)
    {
        mGameContext.mSceneStack->popScene(Transition{.mTransitionType = TransType::GameFromJoinGame});
        mGameContext.mSceneStack->pushScene(
            std::make_shared<GameScene>(mGameContext, mMappingContext));
        return;
    }

    mSystems.get(update)->get<system::SceneGraphResolver>().update();
    mSystems.get(update)->get<system::AdvanceAnimations>().update(aTime);
}

void JoinGameScene::onEnter(Transition aTransition) {
    // There is 3 possible transition to JoinGameScene
    // From the main menu (press start)
    // From the pause (press quit)
    // From the podium (press change players)
    mSystems = mGameContext.mWorld.addEntity("System for JoinGame");
    Phase init;
    mSystems.get(init)
        ->add(system::SceneGraphResolver{mGameContext, mGameContext.mSceneRoot})
        .add(system::AdvanceAnimations{mGameContext})
        .add(system::InputProcessor{mGameContext, mMappingContext});

    switch (aTransition.mTransitionType)
    {
        case TransType::JoinGameFromMenu:
        {
            // We need to create a player slot for the player
            // that pressed start
            JoinGameSceneInfo info = std::get<JoinGameSceneInfo>(aTransition.mSceneInfo);
            addPlayerFromControllerId(info.mTransitionControllerId);
            break;
        }
        case TransType::JoinGameFromPause:
        {
            // From game transition there is no need to create a player slot
            // we can just use the existing slot
            mSlots.each([this](EntHandle aSlotHandle, component::PlayerSlot & aSlot) {
                createPlayerModelFromSlot(aSlotHandle, aSlot);
            });
            break;
        }
        case TransType::JoinGameFromPodium:
        {
            mSlots.each([this](EntHandle aSlotHandle, component::PlayerSlot & aSlot) {
                createPlayerModelFromSlot(aSlotHandle, aSlot);
            });
            break;
        }
        default:
            break;   
    }
}

void JoinGameScene::onExit(Transition aTransition) {
    // There is 2 possible transition to JoinGameScene
    // To the game
    // To the menu
    Phase onExitPhase;
    eraseEntityRecursive(mJoinGameRoot, onExitPhase);
    if (aTransition.mTransitionType == TransType::QuitToMenu)
    {
        snac::eraseAllInQuery(mSlots);
    }
    mSystems.get(onExitPhase)->erase();
}

void JoinGameScene::createPlayerModelFromSlot(
    ent::Handle<ent::Entity> & aSlotHandle,
    const component::PlayerSlot & aSlot)
{
    EntHandle joinGamePlayer = createJoinGamePlayer(mGameContext, aSlotHandle, (int)aSlot.mSlotIndex);
    insertEntityInScene(joinGamePlayer, mJoinGameRoot);
    Phase joinPlayer;
    aSlotHandle.get(joinPlayer)->add(component::PlayerJoinData{.mJoinPlayerModel = joinGamePlayer});
}

void JoinGameScene::addPlayerFromControllerId(int controllerId)
{
    EntHandle slotHandle = addPlayer(mGameContext, controllerId);
    const component::PlayerSlot & slot =
        slotHandle.get()->get<component::PlayerSlot>();
    createPlayerModelFromSlot(slotHandle, slot);
}

} // namespace scene
} // namespace snacgame
} // namespace ad
