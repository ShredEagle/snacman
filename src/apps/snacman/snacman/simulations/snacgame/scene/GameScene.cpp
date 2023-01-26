#include "GameScene.h"
#include "snacman/simulations/snacgame/system/LevelCreator.h"

#include "../component/Context.h"
#include "../component/LevelData.h"
#include "../system/DeterminePlayerAction.h"
#include "../system/IntegratePlayerMovement.h"
#include "../system/PlayerInvulFrame.h"
#include "../system/PlayerSpawner.h"

namespace ad {
namespace snacgame {
namespace scene {

void GameScene::setup(const Transition & aTransition)
{
    ent::Phase init;
    mSystems.get(init)->add(system::PlayerSpawner{mWorld});
    mSystems.get(init)->add(system::PlayerInvulFrame{mWorld});
    mSystems.get(init)->add(system::DeterminePlayerAction{mWorld, mLevel});
    mSystems.get(init)->add(system::IntegratePlayerMovement{mWorld, mLevel});
    mSystems.get(init)->add(system::LevelCreator{&mWorld});

    mLevel.get(init)->add(component::LevelData(
        mWorld, gAssetRoot, "markov/snaclvl.xml", {29, 29, 1}, 123123));
    mLevel.get(init)->add(component::LevelToCreate{});
}

void GameScene::teardown() {}

std::optional<Transition> GameScene::update(float aDelta,
                                            const RawInput & aInput)
{
    ent::Phase update;
    mSystems.get(update)->get<system::LevelCreator>().update();
    /* ent::Phase update; */
    /* InputDeviceDirectory & directory = mContext->mInputDeviceDirectory; */

    /* for (auto & connectedPlayer : directory.mGamepadBindings) */
    /* { */
    /*     if (connectedPlayer.mPlayer && connectedPlayer.mPlayer->isValid()) */
    /*     { */
    /*         component::Controller & controller = */
    /*             connectedPlayer.mPlayer->get(update) */
    /*                 ->get<component::Controller>(); */
    /*         controller.mCommandQuery = convertGamepadInput( */
    /*             "player", */
    /*             aInput.mGamepads.at(connectedPlayer.mRawInputGamepadIndex), */
    /*             GamepadMapping(mContext->mGamepadMapping)); */
    /*     } */
    /* } */

    /* auto & keyboardPlayer = directory.mPlayerBoundToKeyboard; */

    /* if (keyboardPlayer && keyboardPlayer->isValid()) */
    /* { */
    /*     component::Controller & keyboard = */
    /*         keyboardPlayer->get(update)->get<component::Controller>(); */

    /*     keyboard.mCommandQuery = */
    /*         convertKeyboardInput("player", aInput.mKeyboard, */
    /*                              KeyboardMapping(mContext->mKeyboardMapping)); */

    /*     quitGame = keyboard.mCommandQuery & gQuitCommand; */
    /* } */

    /* mSystems.get(update)->get<system::PlayerSpawner>().update(aDelta); */
    /* mSystems.get(update)->get<system::PlayerInvulFrame>().update(aDelta); */
    /* mSystems.get(update)->get<system::DeterminePlayerAction>().update(); */
    /* mSystems.get(update)->get<system::IntegratePlayerMovement>().update(aDelta); */

    return std::nullopt;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
