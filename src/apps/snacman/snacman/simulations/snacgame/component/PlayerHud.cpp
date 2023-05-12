#include "PlayerHud.h"


#include "PlayerLifeCycle.h"
#include "PlayerPowerUp.h"
#include "PlayerSlot.h"


namespace ad {
namespace snacgame {
namespace component {


int PlayerHud::getScore() const
{
    auto playerView = mPlayer.get();
    assert(playerView && playerView->has<PlayerLifeCycle>());
    return playerView->get<PlayerLifeCycle>().mScore;
}


const PlayerSlot & PlayerHud::getSlot() const
{
    auto playerView = mPlayer.get();
    assert(playerView && playerView->has<PlayerSlot>());
    return playerView->get<PlayerSlot>();
}


const char * PlayerHud::getPowerUpName() const
{
    auto playerView = mPlayer.get();
    assert(playerView);
    
    // In the current design, the PlayerPowerUp component is only present while a power up is picked up
    if(playerView->has<PlayerPowerUp>())
    {
        return component::gPowerUpName.at(
            static_cast<std::size_t>(playerView->get<PlayerPowerUp>().mType));
    }
    else
    {
        return "";
    }
}


} // namespace component
} // namespace snacgame
} // namespace ad
