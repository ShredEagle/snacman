#pragma once

#include "Scene.h"
#include "snacman/simulations/snacgame/component/LevelData.h"

#include "../component/Controller.h"
#include "../component/PlayerSlot.h"

#include <entity/Query.h>
#include <entity/EntityManager.h>
#include <string>

namespace ad {
namespace snacgame {
namespace scene {

class GameScene : public Scene
{
public:
    GameScene(const std::string & aName,
              ent::EntityManager & aWorld,
              EntityWrap<component::Context> & aContext) :
        Scene(aName, aWorld, aContext), mLevel{mWorld.addEntity()},
        mTiles{mWorld},
        mSlots{mWorld},
        mPlayers{mWorld}
    {}

    std::optional<Transition> update(float aDelta,
                                     RawInput & aInput) override;
    void setup(const Transition & aTransition) override;
    void teardown() override;

private:
    ent::Handle<ent::Entity> mLevel;
    ent::Query<component::LevelEntity> mTiles;
    ent::Query<component::PlayerSlot> mSlots;
    ent::Query<component::PlayerSlot, component::Controller> mPlayers;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
