#include "PlayerHud.h"

#include "PlayerRoundData.h"


namespace ad {
namespace snacgame {
namespace component {


std::string getPowerUpName(ent::Handle<ent::Entity> aPlayer)
{
    auto playerView = aPlayer.get();
    assert(playerView);
    
    // In the current design, the PlayerPowerUp component is only present while a power up is picked up
    if(playerView->has<PlayerRoundData>())
    {
        return component::gPowerUpName.at(
            static_cast<std::size_t>(playerView->get<PlayerRoundData>().mType));
    }
    else
    {
        return "";
    }
}


} // namespace component
} // namespace snacgame
} // namespace ad
