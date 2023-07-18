#include "InputProcessor.h"

#include "entity/Wrap.h"
#include "snacman/Input.h"
#include "snacman/simulations/snacgame/InputCommandConverter.h"
#include "snacman/simulations/snacgame/component/Context.h"

#include "../component/Controller.h"
#include "../component/PlayerSlot.h"
#include "../GameContext.h"
#include <algorithm>

namespace ad {
namespace snacgame {
namespace system {
InputProcessor::InputProcessor(
    GameContext & aGameContext,
    ent::Wrap<component::MappingContext> & aMappingContext) :
    mPlayers{aGameContext.mWorld}, mMappingContext{&aMappingContext}
{}

std::vector<ControllerCommand>
InputProcessor::mapControllersInput(RawInput & aInput)
{
    std::vector<ControllerCommand> controllers;

    mPlayers.each(
        [&](component::Controller & aController)
        {
        switch (aController.mType)
        {
        case ControllerType::Keyboard:
            aController.mInput = convertKeyboardInput(
                "player", aInput.mKeyboard, (*mMappingContext)->mKeyboardMapping);
            break;
        case ControllerType::Gamepad:
            aController.mInput = convertGamepadInput(
                "player", aInput.mGamepads.at(aController.mControllerId),
                (*mMappingContext)->mGamepadMapping);
            break;
        default:
            break;
        }

        controllers.push_back({(int)aController.mControllerId,
                                      aController.mType, aController.mInput,
                                      true});
    });

    for (int controlIndex = 0; controlIndex < (int) aInput.mGamepads.size() + 1;
         ++controlIndex)
    {
        if (std::find_if(controllers.begin(), controllers.end(),
                      [controlIndex](const ControllerCommand & aCommand) {
                          return aCommand.mId == controlIndex;
                      })
            == controllers.end())
        {
            GameInput input{.mCommand = gNoCommand};
            const bool controllerIsKeyboard =
                (int) controlIndex == gKeyboardControllerIndex;

            if (controllerIsKeyboard)
            {
                input = convertKeyboardInput("unbound", aInput.mKeyboard,
                                             (*mMappingContext)->mKeyboardMapping);
                controllers.push_back({gKeyboardControllerIndex, ControllerType::Keyboard, input, false});
            }
            else
            {
                GamepadState & rawGamepad = aInput.mGamepads.at(controlIndex);
                input = convertGamepadInput("unbound", rawGamepad,
                                            (*mMappingContext)->mGamepadMapping);
                controllers.push_back({controlIndex, ControllerType::Keyboard, input, false});
            }
        }
    }

    return controllers;
}
} // namespace system
} // namespace snacgame
} // namespace ad
