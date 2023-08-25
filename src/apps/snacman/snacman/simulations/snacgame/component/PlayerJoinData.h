#pragma once

#include <entity/Entity.h>

namespace ad {
namespace snacgame {
namespace component {
struct PlayerJoinData
{
    ent::Handle<ent::Entity> mJoinPlayerModel;
};
} // namespace component
} // namespace snacgame
} // namespace ad
