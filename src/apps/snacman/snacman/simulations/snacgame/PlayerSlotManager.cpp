#include "PlayerSlotManager.h"

#include "GameContext.h"
#include "Entities.h"
#include "typedef.h"

#include "component/PlayerGameData.h"
#include "component/PlayerSlot.h"

namespace ad {
namespace snacgame {

const std::set<unsigned int> gFreeIndicesSet = {1, 2, 3, 4};
constexpr int gNoSlotAvailable = -1;

PlayerSlotManager::PlayerSlotManager(GameContext * aContext) :
    mPlayerSlots{aContext->mWorld}
{}

bool PlayerSlotManager::addPlayer(GameContext & aContext, EntHandle aPlayer,
                                  unsigned int aControllerIndex)
{
    ent::Phase addPlayer;
    if (mPlayerSlots.countMatches() < gMaxPlayerSlots)
    {
        int slotIndex = findAvailableSlotIndex(aControllerIndex);

        if (slotIndex > -1)
        {
            aPlayer.get(addPlayer)
                ->add(component::PlayerSlot{
                        .mSlotIndex = (unsigned int)slotIndex,
                        .mControllerIndex = aControllerIndex,
                        .mControllerType = aControllerIndex == gKeyboardControllerIndex ? ControllerType::Keyboard : ControllerType::Gamepad
                        })
                .add(component::Unspawned{})
                .add(component::PlayerGameData{
                        .mHud = createHudBillpad(aContext, slotIndex),
                        });
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
        {
        lastUsedIndexAvailable &= lastUsedIndex != aSlot.mSlotIndex;
    });

    if (lastUsedIndexAvailable)
    {
        return lastUsedIndex;
    }
    
    std::set<unsigned int> freeIndices = gFreeIndicesSet;

    mPlayerSlots.each(
        [&freeIndices](const component::PlayerSlot & aSlot)
        {
        freeIndices.erase(aSlot.mSlotIndex);
    });

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
