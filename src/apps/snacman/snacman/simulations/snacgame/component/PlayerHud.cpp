#include "PlayerHud.h"

#include "PlayerPowerUp.h"


namespace ad {
namespace snacgame {
namespace component {


const char * getPowerUpName(ent::Handle<ent::Entity> aPlayer)
{
    auto playerView = aPlayer.get();
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
