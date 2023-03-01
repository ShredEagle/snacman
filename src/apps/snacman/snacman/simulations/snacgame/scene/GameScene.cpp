#include "GameScene.h"

#include "snacman/Input.h"
#include <snacman/Logging.h>
#include <snacman/Profiling.h>
#include "snacman/simulations/snacgame/component/Controller.h"
#include "snacman/simulations/snacgame/InputCommandConverter.h"
#include "snacman/simulations/snacgame/component/PoseScreenSpace.h"
#include "snacman/simulations/snacgame/component/VisualMesh.h"
#include "snacman/simulations/snacgame/system/LevelCreator.h"
#include "snacman/simulations/snacgame/system/MovementIntegration.h"

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

#include <algorithm>

namespace ad {
namespace snacgame {
namespace scene {

void GameScene::setup(GameContext & aContext, const Transition & aTransition, RawInput & aInput)
{
    ent::Phase init;
    mSystems.get(init)->add(system::PlayerSpawner{mWorld});
    mSystems.get(init)->add(system::PlayerInvulFrame{mWorld});
    mSystems.get(init)->add(system::DeterminePlayerAction{mWorld, mLevel});
    mSystems.get(init)->add(system::IntegratePlayerMovement{mWorld, mLevel});
    mSystems.get(init)->add(system::LevelCreator{&mWorld});
    mSystems.get(init)->add(system::MovementIntegration{mWorld});

    auto markovRoot = aContext.mResources.find(gMarkovRoot);
    mLevel.get(init)->add(component::LevelData(
        mWorld, markovRoot.value(), "snaclvl.xml", {29, 29, 1}, 123123));
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

    mPlayers.each([&destroy](ent::Handle<ent::Entity> aHandle,
                             component::PlayerSlot & aSlot,
                             component::Controller & aController) {
        aSlot.mFilled = false;

        aHandle.get(destroy)->remove<component::Controller>();
        aHandle.get(destroy)->remove<component::Geometry>();
        aHandle.get(destroy)->remove<component::PlayerLifeCycle>();
        aHandle.get(destroy)->remove<component::PlayerMoveState>();
        aHandle.get(destroy)->remove<component::Text>();
        aHandle.get(destroy)->remove<component::VisualMesh>();
        aHandle.get(destroy)->remove<component::PoseScreenSpace>();
    });
}

std::optional<Transition> GameScene::update(GameContext & aContext,
                                            float aDelta,
                                            RawInput & aInput)
{
    TIME_RECURRING(Main, "GameScene::update");

    ent::Phase update;

    bool quit = false;

    std::vector<int> boundControllers;

    mPlayers.each([&](component::PlayerSlot & aPlayerSlot,
                      component::Controller & aController) {
        switch (aController.mType)
        {
        case ControllerType::Keyboard:
            aController.mCommandQuery = convertKeyboardInput(
                "player", aInput.mKeyboard, mContext->mKeyboardMapping);
            break;
        case ControllerType::Gamepad:
            aController.mCommandQuery = convertGamepadInput(
                "player", aInput.mGamepads.at(aController.mControllerId),
                mContext->mGamepadMapping);
            break;
        default:
            break;
        }

        boundControllers.push_back(aController.mControllerId);

        quit |= static_cast<bool>(aController.mCommandQuery & gQuitCommand);
    });

    ent::Phase bindPlayerPhase;

    if (std::find(boundControllers.begin(), boundControllers.end(),
                  gKeyboardControllerIndex)
        == boundControllers.end())
    {
        int keyboardCommand = convertKeyboardInput("unbound", aInput.mKeyboard,
                                                   mContext->mKeyboardMapping);

        if (keyboardCommand & gJoin)
        {
            findSlotAndBind(aContext, bindPlayerPhase, mSlots, ControllerType::Keyboard,
                            gKeyboardControllerIndex);
        }
    }

    for (std::size_t index = 0; index < aInput.mGamepads.size(); ++index)
    {
        if (std::find(boundControllers.begin(), boundControllers.end(),
                      index)
            == boundControllers.end())
        {
            GamepadState & rawGamepad = aInput.mGamepads.at(index);
            int gamepadCommand =
                convertGamepadInput("unbound", rawGamepad, mContext->mGamepadMapping);

            if (gamepadCommand & gJoin)
            {
                findSlotAndBind(aContext, bindPlayerPhase, mSlots,
                                               ControllerType::Gamepad, static_cast<int>(index));
            }
        }
    }

    if (quit)
    {
        return Transition{.mTransitionName = "back"};
    }

    mSystems.get(update)->get<system::LevelCreator>().update(aContext);
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
