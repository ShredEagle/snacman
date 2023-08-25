#pragma once

#include <entity/Query.h>

namespace ad {
namespace snacgame {
struct ControllerCommand;
struct GameContext;
namespace component {
struct MenuItem;
struct Text;
}
namespace system {

class MenuManager
{
public:
    MenuManager(GameContext & aGameContext);

    std::pair<component::MenuItem, int> manageMenu(const std::vector<ControllerCommand> & controllerCommands);

private:
    GameContext * mGameContext;
    ent::Query<component::MenuItem, component::Text> mMenuItems;
};

} // namespace system
} // namespace snacgame
} // namespace ad
