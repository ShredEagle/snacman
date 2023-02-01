#pragma once

#include "Scene.h"
#include "entity/Query.h"
#include "snacman/Input.h"
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
              EntityWrap<component::MappingContext> & aContext) :
        Scene(aName, aWorld, aContext), mSlots{mWorld}
    {}

    std::optional<Transition> update(GameContext & aContext,
                                     float aDelta,
                                     RawInput & aInput) override;
    void setup(const Transition & aTransition, RawInput & aInput) override;
    void teardown(RawInput & aInput) override;
private:
    ent::Query<component::PlayerSlot> mSlots;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
