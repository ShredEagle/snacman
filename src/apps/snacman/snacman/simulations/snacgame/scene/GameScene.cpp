#include "GameScene.h"

#include "snacman/Logging.h"

#include "../component/Context.h"
#include "../component/Controller.h"
#include "../component/LevelData.h"
#include "../Entities.h"
#include "../InputCommandConverter.h"
#include "../system/DeterminePlayerAction.h"
#include "../system/IntegratePlayerMovement.h"
#include "../system/LevelCreator.h"
#include "../system/MovementIntegration.h"
#include "../system/PlayerInvulFrame.h"
#include "../system/PlayerSpawner.h"

namespace ad {
namespace snacgame {
namespace scene {

void GameScene::setup(const Transition & aTransition, RawInput & aInput)
{
    ent::Phase init;
    mSystems.get(init)->add(system::PlayerSpawner{mWorld});
    mSystems.get(init)->add(system::PlayerInvulFrame{mWorld});
    mSystems.get(init)->add(system::DeterminePlayerAction{mWorld, mLevel});
    mSystems.get(init)->add(system::IntegratePlayerMovement{mWorld, mLevel});
    mSystems.get(init)->add(system::LevelCreator{&mWorld});
    mSystems.get(init)->add(system::MovementIntegration{mWorld});

    mLevel.get(init)->add(component::LevelData(
        mWorld, gAssetRoot, "markov/snaclvl.xml", {29, 29, 1}, 123123));
    mLevel.get(init)->add(component::LevelToCreate{});
}

void GameScene::teardown(RawInput & aInput)
{
    ent::Phase destroy;

    mSystems.get(destroy)->erase();
    mSystems = mWorld.addEntity();

    mLevel.get(destroy)->erase();
    mLevel = mWorld.addEntity();

    mTiles.each([&destroy](ent::Handle<ent::Entity> aHandle,
                           const component::LevelEntity &) {
        aHandle.get(destroy)->erase();
    });

    mPlayers.each([&destroy, &aInput](ent::Handle<ent::Entity> aHandle,
                                      component::PlayerSlot & aSlot,
                                      component::Controller & aController) {
        aSlot.mFilled = false;
        if (aController.mType == component::ControllerType::Keyboard)
        {
            aInput.mKeyboard.mBound = false;
        }
        else if (aController.mType == component::ControllerType::Gamepad)
        {
            aInput.mGamepads.at(aController.mControllerId).mBound = false;
        }
        aHandle.get(destroy)->remove<component::Controller>();
        aHandle.get(destroy)->remove<component::Geometry>();
        aHandle.get(destroy)->remove<component::PlayerLifeCycle>();
        aHandle.get(destroy)->remove<component::PlayerMoveState>();
    });
}

std::optional<Transition> GameScene::update(float aDelta, RawInput & aInput)
{
    ent::Phase update;

    bool quit = false;

    mSlots.each([&](ent::Handle<ent::Entity> aHandle,
                    component::PlayerSlot & aPlayerSlot) {
        if (!aPlayerSlot.mFilled)
        {
            if (!aInput.mKeyboard.mBound)
            {
                int commandQuery = convertKeyboardInput("unbound",
                        aInput.mKeyboard, mContext->mKeyboardMapping);
                if (commandQuery & gJoin)
                {
                    fillSlotWithPlayer(update, component::ControllerType::Keyboard, aHandle, 0);
                    aInput.mKeyboard.mBound = true;
                }
            }
            else
            {
                for (std::size_t index = 0; index < aInput.mGamepads.size();
                     ++index)
                {
                    GamepadState & gamepad = aInput.mGamepads.at(index);
                    if (!gamepad.mBound)
                    {
                        int commandQuery = convertGamepadInput("unbound",
                                gamepad, mContext->mGamepadMapping);
                        if (commandQuery & gJoin)
                        {
                            fillSlotWithPlayer(update, component::ControllerType::Gamepad, aHandle, index);
                            gamepad.mBound = true;
                        }
                    }
                }
            }
        }
    });

    mPlayers.each([&](component::PlayerSlot & aPlayerSlot,
                      component::Controller & aController) {
        switch (aController.mType)
        {
        case component::ControllerType::Keyboard:
            aController.mCommandQuery = convertKeyboardInput(
                "player", aInput.mKeyboard, mContext->mKeyboardMapping);
            break;
        case component::ControllerType::Gamepad:
            aController.mCommandQuery = convertGamepadInput(
                "player", aInput.mGamepads.at(aController.mControllerId),
                mContext->mGamepadMapping);
            break;
        default:
            break;
        }

        quit |= static_cast<bool>(aController.mCommandQuery & gQuitCommand);
    });

    if (quit)
    {
        return Transition{.mTransitionName = "back"};
    }

    mSystems.get(update)->get<system::LevelCreator>().update();
    mSystems.get(update)->get<system::PlayerSpawner>().update(aDelta);
    mSystems.get(update)->get<system::PlayerInvulFrame>().update(aDelta);
    mSystems.get(update)->get<system::DeterminePlayerAction>().update();
    mSystems.get(update)->get<system::IntegratePlayerMovement>().update(aDelta);
    mSystems.get(update)->get<system::MovementIntegration>().update(aDelta);

    return std::nullopt;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
