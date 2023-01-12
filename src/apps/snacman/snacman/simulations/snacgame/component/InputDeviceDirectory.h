#pragma once
#include "Controller.h"
#include "entity/Entity.h"

#include "../InputCommandConverter.h"

namespace ad {
namespace snacgame {
namespace component {

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
                .mType = ControllerType::Gamepad,
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
            .mType = ControllerType::Keyboard,
        });
        mPlayerBoundToKeyboard = aPlayer;
    }

    std::array<GamepadPlayerBinding, gMaxControllerListSize> mGamepadBindings;
    std::optional<ent::Handle<ent::Entity>> mPlayerBoundToKeyboard;
};

} // namespace component
} // namespace snacgame
} // namespace ad
