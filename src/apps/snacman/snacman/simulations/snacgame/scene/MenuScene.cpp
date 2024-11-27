#include "MenuScene.h"

#include "snacman/serialization/Witness.h"
#include "snacman/simulations/snacgame/scene/Scene.h"
#include "snacman/simulations/snacgame/system/InputProcessor.h"
#include "snacman/simulations/snacgame/system/MenuManager.h"
#include "snacman/simulations/snacgame/system/SystemOrbitalCamera.h"

#include "../component/Context.h"
#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/MenuItem.h"
#include "../component/PathToOnGrid.h"
#include "../component/PlayerHud.h"
#include "../component/PlayerRoundData.h"
#include "../component/PlayerSlot.h"
#include "../component/SceneNode.h"
#include "../component/Tags.h"
#include "../component/Text.h"
#include "../Entities.h"
#include "../GameParameters.h"
#include "../InputCommandConverter.h"
#include "../OrbitalControlInput.h"
#include "../scene/DataScene.h"
#include "../scene/GameScene.h"
#include "../scene/JoinGameScene.h"
#include "../SceneGraph.h"
#include "../SnacGame.h"
#include "../system/SceneGraphResolver.h"
#include "../typedef.h"

#include <entity/EntityManager.h>
#include <optional>
#include <snacman/EntityUtilities.h>
#include <snacman/Input.h>
#include <snacman/Logging.h>
#include <snacman/Profiling.h>
#include <snacman/QueryManipulation.h>
#include <snacman/Timing.h>
#include <string>

namespace ad {
namespace snacgame {
namespace scene {

constexpr math::Position<3, float> gMenuSceneInitialCameraPos{
    0.f, 0.f, 0.f
};

MenuScene::MenuScene(GameContext & aGameContext,
                     ent::Wrap<component::MappingContext> & aContext) :
    Scene(gMenuSceneName, aGameContext, aContext), mItems{mGameContext.mWorld}
{
    auto font = mGameContext.mResources.getFont("fonts/TitanOne-Regular.ttf");

    serial::EntityLedger menuLedger = serial::loadLedgerFromJson(
        "scenes/menu_scene.json", mGameContext.mWorld, mGameContext);

    //TODO(franz): this should change on ledger are a bit more integrated
    mOwnedEntities = menuLedger.mEntities;

    EntHandle camera = snac::getFirstHandle(mCameraQuery);
    OrbitalControlInput & orbitalControl =
        snac::getComponent<system::OrbitalCamera>(camera).mControl;
    orbitalControl.mOrbital.mSpherical = gInitialCameraSpherical;
    orbitalControl.mOrbital.mSphericalOrigin = gMenuSceneInitialCameraPos;
}

void MenuScene::onEnter(Transition aTransition) {
    mSystems = mGameContext.mWorld.addEntity("System for Menu");

    ent::Phase init;
    mSystems.get(init)
        ->add(system::SceneGraphResolver{mGameContext, mGameContext.mSceneRoot})
        .add(system::MenuManager{mGameContext})
        .add(system::InputProcessor{mGameContext, mMappingContext});

}

void MenuScene::onExit(Transition aTransition)
{
    ent::Phase destroy;

    for (auto handle : mOwnedEntities)
    {
        handle.get(destroy)->erase();
    }

    mSystems.get(destroy)->erase();
}

void MenuScene::update(const snac::Time & aTime, RawInput & aInput)
{
    TIME_RECURRING_CLASSFUNC(Main);

    std::vector<ControllerCommand> controllerCommands =
        mSystems.get()->get<system::InputProcessor>().mapControllersInput(
            aInput, "menu", "menu");

    auto [menuItem, accumulatedCommand] =
        mSystems.get()->get<system::MenuManager>().manageMenu(
            controllerCommands);

    bool quit = accumulatedCommand & gQuitCommand;

    if (menuItem.mTransition.mTransitionType
        == TransType::JoinGameFromMenu)
    {
        for (auto command : controllerCommands)
        {
            if (command.mInput.mCommand & gSelectItem)
            {
                Transition menuTransition = menuItem.mTransition;
                menuTransition.mSceneInfo = {JoinGameSceneInfo{command.mId}};
                // Menu pops itself from the stack
                mGameContext.mSceneStack->popScene();
                mGameContext.mSceneStack->pushScene(
                    std::make_shared<JoinGameScene>(mGameContext,
                                                    mMappingContext),
                    menuTransition);
                return;
            }
        }
    }
    else if (menuItem.mTransition.mTransitionType == TransType::QuitAll
             && accumulatedCommand & gSelectItem)
    {
        quit = true;
    }

    if (quit)
    {
        // Menu pops itself from the stack
        mGameContext.mSceneStack->popScene(
            Transition{.mTransitionType = TransType::QuitAll});
        return;
    }

    mSystems.get()->get<system::SceneGraphResolver>().update();

    return;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
