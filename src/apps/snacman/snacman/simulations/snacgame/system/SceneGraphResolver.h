#pragma once

#include "../component/Geometry.h"
#include "../component/SceneRoot.h"
#include "../component/GlobalPosition.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {
class SceneGraphResolver
{
    SceneGraphResolver(ent::EntityManager & aWorld) :
        mRoots{aWorld}
    {}

    void update();
private:
    ent::Query<component::SceneRoot, component::Geometry, component::GlobalPosition> mRoots;
};
}
} // namespace snacgame
} // namespace ad
