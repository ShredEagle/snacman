#pragma once

#include "entity/Entity.h"
#include "math/Vector.h"
namespace ad {
namespace snacgame {
namespace component {
    
struct PlayerPortalData
{
    std::optional<ent::Handle<ent::Entity>> mPortalImage;
    math::Position<2, float> mCurrentPortalPos;
    int mCurrentPortal = -1;
    int mDestinationPortal = -1;
};
}
}
}
