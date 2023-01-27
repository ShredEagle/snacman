#pragma once

#include "Scene.h"
#include "entity/Query.h"
#include "snacman/simulations/snacgame/component/PlayerSlot.h"

#include <string>

namespace ad {
namespace snacgame {
namespace scene {

class MenuScene : public Scene
{
public:
    MenuScene(const std::string & aName,
              ent::EntityManager & aWorld,
              EntityWrap<component::Context> & aContext) :
        Scene(aName, aWorld, aContext), mSlots{mWorld}
    {}

    std::optional<Transition> update(float aDelta,
                                     RawInput & aInput) override;
    void setup(const Transition & aTransition) override;
    void teardown() override;
private:
    ent::Query<component::PlayerSlot> mSlots;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
