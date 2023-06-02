#include "PlayerSlotManager.h"

#include "component/PlayerGameData.h"
#include "component/PlayerSlot.h"

namespace ad {
namespace snacgame {

bool PlayerSlotManager::addPlayer(ent::Handle<ent::Entity> aPlayer,
                                  unsigned int aControllerIndex)
{
    ent::Phase addPlayer;
    if (mPlayerSlots.countMatches() < gMaxPlayerSlots)
    {
        unsigned int slotIndex = findAvailableSlotIndex(aControllerIndex);
        aPlayer.get(addPlayer)
            ->add(component::PlayerSlot{})
            .add(component::PlayerGameData{});
        return true;
    }

    return false;
}

unsigned int PlayerSlotManager::findAvailableSlotIndex(unsigned int aIndex)
{
    unsigned int lastUsedIndex = aIndex;
    bool lastUsedIndexAvailable = true;

    if (mControllerIndexLastUsedSlot.contains(aIndex))
    {
        lastUsedIndex = mControllerIndexLastUsedSlot.at(aIndex);
    }

    mPlayerSlots.each(
        [lastUsedIndex,
         &lastUsedIndexAvailable](const component::PlayerSlot & aSlot)
        {
        lastUsedIndexAvailable &= lastUsedIndex != aSlot.mIndex;
    });

    if (lastUsedIndexAvailable)
    {
        return lastUsedIndex;
    }
    
    unsigned int freeIndex;

    for (unsigned int i = 0; i < gMaxPlayerSlots; ++i)
    {
        mPlayerSlots.each(
            [lastUsedIndex,
             &lastUsedIndexAvailable](const component::PlayerSlot & aSlot)
            {
            lastUsedIndexAvailable &= lastUsedIndex != aSlot.mIndex;
        });
    }

    mControllerIndexLastUsedSlot.insert({aIndex, freeIndex});

    return freeIndex;
}

} // namespace snacgame
} // namespace ad
