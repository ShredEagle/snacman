#include "MenuManager.h"

#include "snacman/simulations/snacgame/InputCommandConverter.h"
#include "snacman/simulations/snacgame/InputConstants.h"
#include "../component/MenuItem.h"
#include "../component/Text.h"

#include "../GameContext.h"
#include "../typedef.h"

#include <snacman/QueryManipulation.h>
#include <snacman/Logging.h>

namespace ad {
namespace snacgame {
namespace system {

MenuManager::MenuManager(GameContext & aGameContext) :
    mMenuItems{aGameContext.mWorld}
{}

std::pair<component::MenuItem, int> MenuManager::manageMenu(const std::vector<ControllerCommand> & controllerCommands)
{
    std::string newItem;

    int accumulatedCommand = 0;

    for (auto controller : controllerCommands)
    {
        accumulatedCommand |= controller.mInput.mCommand;
    }

    // Really dirty way to have positiveEdge on joystick axis
    int filteredForMenuCommand =
        ((accumulatedCommand & (gGoUp | gGoDown | gGoLeft | gGoRight | gPositiveEdge)) ^ mLastInput) & accumulatedCommand;

    EntHandle selectedItemHandle = snac::getFirstHandle(
        mMenuItems, [](component::MenuItem & aItem) { return aItem.mSelected; });
    component::MenuItem & menuItem =
        snac::getComponent<component::MenuItem>(selectedItemHandle);
    component::Text & text =
        snac::getComponent<component::Text>(selectedItemHandle);

    if (menuItem.mNeighbors.contains(filteredForMenuCommand))
    {
        newItem = menuItem.mNeighbors.at(filteredForMenuCommand);
        menuItem.mSelected = false;
        text.mColor = gColorItemUnselected;
    }

    bool newItemFound = false;
    mMenuItems.each(
        [&newItem, &newItemFound](component::MenuItem & aItem,
                                  component::Text & aText)
        {
        if (aItem.mName == newItem)
        {
            newItemFound = true;
            aItem.mSelected = true;
            aText.mColor = gColorItemSelected;
        }
    });

    if (!newItemFound && newItem != "")
    {
        SELOG(error)
        ("Could not find item {} in menu (Check the case)", newItem);
    }

    mLastInput = accumulatedCommand & (gGoUp | gGoDown | gGoLeft | gGoRight);


    return {menuItem, accumulatedCommand};
}

}
}}
