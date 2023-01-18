#pragma once

#include "../component/Controller.h"
#include "../InputCommandConverter.h"

#include <entity/Entity.h>

namespace ad {
namespace snacgame {

constexpr int gMaxControllerListSize = 4;
constexpr int gPlayerSlotUnavailable = -1;

struct GamepadPlayerBinding
{
    int mRawInputGamepadIndex = -1;
    std::optional<ent::Handle<ent::Entity>> mPlayer;
};

struct InputDeviceDirectory
{
    void bindPlayerToGamepad(ent::Phase & aPhase,
                             ent::Handle<ent::Entity> & aPlayer,
                             int aRawInputGamepadIndex)
    {
        int playerSlot = 0;
        for (auto & binding : mGamepadBindings)
        {
            if (binding.mPlayer && binding.mPlayer->isValid())
            {
                continue;
            }

            aPlayer.get(aPhase)->add(component::Controller{
                .mType = component::ControllerType::Gamepad,
                .mId = playerSlot,
            });
            binding.mPlayer = aPlayer;
            binding.mRawInputGamepadIndex = aRawInputGamepadIndex;

            return;
        }
    }

    void bindPlayerToKeyboard(ent::Phase & aPhase,
                              ent::Handle<ent::Entity> & aPlayer)
    {
        if (mPlayerBoundToKeyboard && mPlayerBoundToKeyboard->isValid())
        {
            return;
        }

        aPlayer.get(aPhase)->add(component::Controller{
            .mType = component::ControllerType::Keyboard,
        });
        mPlayerBoundToKeyboard = aPlayer;
    }

    std::array<GamepadPlayerBinding, gMaxControllerListSize> mGamepadBindings;
    std::optional<ent::Handle<ent::Entity>> mPlayerBoundToKeyboard;
};

} // namespace snacgame
} // namespace ad
