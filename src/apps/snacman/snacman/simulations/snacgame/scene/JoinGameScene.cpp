#include "JoinGameScene.h"

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

#include "snacman/EntityUtilities.h"
#include "snacman/simulations/snacgame/Entities.h"
#include "snacman/simulations/snacgame/GameParameters.h"
#include "snacman/simulations/snacgame/InputCommandConverter.h"
#include "snacman/simulations/snacgame/SceneGraph.h"
#include "snacman/simulations/snacgame/component/PlayerJoinData.h"
#include "snacman/simulations/snacgame/scene/GameScene.h"
#include "snacman/simulations/snacgame/scene/MenuScene.h"
#include "snacman/simulations/snacgame/scene/Scene.h"
#include "snacman/simulations/snacgame/system/InputProcessor.h"
#include "snacman/simulations/snacgame/system/SceneGraphResolver.h"

#include <snacman/QueryManipulation.h>

namespace ad {
namespace snacgame {
namespace scene {
JoinGameScene::JoinGameScene(GameContext & aGameContext,
                             ent::Wrap<component::MappingContext> & aMappingContext) :
    Scene(gJoinGameSceneName, aGameContext, aMappingContext),
    mSlots{aGameContext.mWorld},
    mJoinGameRoot{mGameContext.mWorld.addEntity("join game root")}
{
    Phase init;
    mSystems.get(init)
        ->add(system::SceneGraphResolver{mGameContext, mGameContext.mSceneRoot})
        .add(system::InputProcessor{mGameContext, mMappingContext});

    {
        Phase createRoot;
        Entity root = *mJoinGameRoot.get(createRoot);
        addGeoNode(aGameContext, root);
    }

    insertEntityInScene(mJoinGameRoot, mGameContext.mSceneRoot);

    EntHandle camera = snac::getFirstHandle(mCameraQuery);
    snac::Orbital & camOrbital = snac::getComponent<snac::Orbital>(camera);
    camOrbital.mSpherical.polar() = math::Turn<float>{0.22f};
    camOrbital.mSpherical.radius() = 15.f;
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
                EntHandle slotHandle = addPlayer(mGameContext, controller.mId);
                const component::PlayerSlot & slot =
                    slotHandle.get()->get<component::PlayerSlot>();
                EntHandle joinGamePlayer = createJoinGamePlayer(mGameContext, slotHandle, slot.mSlotIndex);
                insertEntityInScene(joinGamePlayer, mJoinGameRoot);
                Phase joinPlayer;
                slotHandle.get(joinPlayer)->add(component::PlayerJoinData{.mJoinPlayerModel = joinGamePlayer});
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
        mGameContext.mSceneStack->popScene(Transition{.mTransitionName = sQuitTransition});
        mGameContext.mSceneStack->pushScene(
            std::make_shared<MenuScene>(mGameContext, mMappingContext));
        return;
    }

    if (start)
    {
        mGameContext.mSceneStack->popScene(Transition{.mTransitionName = sToGameTransition});
        mGameContext.mSceneStack->pushScene(
            std::make_shared<GameScene>(mGameContext, mMappingContext));
        return;
    }

    mSystems.get(update)->get<system::SceneGraphResolver>().update();
}

void JoinGameScene::onEnter(Transition aTransition) {
    if (aTransition.mTransitionName == sFromGameTransition)
    {
        // From game transition there is no need to create a player slot
        // we can just use the existing slot
    }
    else if (aTransition.mTransitionName == sFromMenuTransition)
    {
        // We need to create a player slot for the player
        // that pressed start
        JoinGameSceneInfo info = std::get<JoinGameSceneInfo>(aTransition.mSceneInfo);
        EntHandle slotHandle = addPlayer(mGameContext, info.mTransitionControllerId);
        const component::PlayerSlot & slot =
            slotHandle.get()->get<component::PlayerSlot>();
        EntHandle joinGamePlayer = createJoinGamePlayer(mGameContext, slotHandle, (int)slot.mSlotIndex);
        insertEntityInScene(joinGamePlayer, mJoinGameRoot);
        Phase joinPlayer;
        slotHandle.get(joinPlayer)->add(component::PlayerJoinData{.mJoinPlayerModel = joinGamePlayer});
    }
}

void JoinGameScene::onExit(Transition aTransition) {
    Phase onExitPhase;
    eraseEntityRecursive(mJoinGameRoot, onExitPhase);
    if (aTransition.mTransitionName != sToGameTransition)
    {
        snac::eraseAllInQuery(mSlots);
    }
    mSystems.get(onExitPhase)->erase();
}

} // namespace scene
} // namespace snacgame
} // namespace ad
