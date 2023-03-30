#pragma once

#include "snacman/Input.h"

#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/LevelData.h"
#include "../component/AllowedMovement.h"
#include "../InputCommandConverter.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class AllowMovement
{
public:
    AllowMovement(ent::EntityManager & aWorld,
                          ent::Handle<ent::Entity> aLevel) :
        mMover{aWorld}, mLevel{aWorld}
    {}

    void update();

private:
    ent::Query<component::Geometry,
               component::AllowedMovement>
        mMover;

    ent::Query<component::LevelData, component::LevelCreated> mLevel;
};

} // namespace system
} // namespace snacgame
} // namespace ad
