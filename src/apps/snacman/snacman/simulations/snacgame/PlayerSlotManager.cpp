#include "PlayerSlotManager.h"

#include "component/PlayerGameData.h"
#include "component/PlayerSlot.h"
#include "Entities.h"
#include "GameContext.h"
#include "snacman/Input.h"
#include "snacman/simulations/snacgame/component/Controller.h"
#include "typedef.h"

namespace ad {
namespace snacgame {

const std::set<unsigned int> gFreeIndicesSet = {0, 1, 2, 3};
constexpr int gNoSlotAvailable = -1;

PlayerSlotManager::PlayerSlotManager(GameContext * aContext) :
    mPlayerSlots{aContext->mWorld}
{}

ControllerType getControllerType(unsigned int aControllerId)
{
    if (aControllerId == gKeyboardControllerIndex)
    {
        return ControllerType::Keyboard;
    }
#ifdef SNACMAN_DEVMODE
    else if (aControllerId > 3)
    {
        return ControllerType::Dummy;
    }
#endif
    else
    {
        return ControllerType::Gamepad;
    }
}

bool PlayerSlotManager::addPlayer(GameContext & aContext,
                                  EntHandle aPlayer,
                                  unsigned int aControllerIndex)
{
    ent::Phase addPlayer;
    if (mPlayerSlots.countMatches() < gMaxPlayerSlots)
    {
        int slotIndex = findAvailableSlotIndex(aControllerIndex);

        if (slotIndex > -1)
        {
            ent::Entity playerEnt = *aPlayer.get(addPlayer);

            playerEnt
                .add(component::PlayerSlot{.mSlotIndex =
                                               (unsigned int) slotIndex})
                .add(component::Controller{
                    .mType = getControllerType(aControllerIndex),
                    .mControllerId = aControllerIndex})
                .add(component::PlayerGameData{});

            return true;
        }
    }

    return false;
}

int PlayerSlotManager::findAvailableSlotIndex(unsigned int aControllerId)
{
    unsigned int lastUsedIndex = aControllerId % gMaxPlayerSlots;
    bool lastUsedIndexAvailable = true;

    if (mControllerIndexLastUsedSlot.contains(aControllerId))
    {
        lastUsedIndex = mControllerIndexLastUsedSlot.at(aControllerId);
    }

    mPlayerSlots.each(
        [lastUsedIndex,
         &lastUsedIndexAvailable](const component::PlayerSlot & aSlot)
        { lastUsedIndexAvailable &= lastUsedIndex != aSlot.mSlotIndex; });

    if (lastUsedIndexAvailable)
    {
        return lastUsedIndex;
    }

    std::set<unsigned int> freeIndices = gFreeIndicesSet;

    mPlayerSlots.each([&freeIndices](const component::PlayerSlot & aSlot)
                      { freeIndices.erase(aSlot.mSlotIndex); });

    if (freeIndices.size() > 0)
    {
        unsigned int freeIndex = *freeIndices.begin();
        mControllerIndexLastUsedSlot.insert({aControllerId, freeIndex});
        return freeIndex;
    }

    return gNoSlotAvailable;
}

} // namespace snacgame
} // namespace ad
