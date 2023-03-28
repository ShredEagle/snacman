#pragma once

#include "Scene.h"
#include "math/Color.h"
#include "snacman/Input.h"
#include "snacman/simulations/snacgame/component/Text.h"

#include "../component/MenuItem.h"
#include "../component/PlayerSlot.h"

#include <entity/Query.h>
#include <string>

namespace ad {
namespace snacgame {
namespace scene {

inline const math::hdr::Rgba_f gColorItemUnselected = math::hdr::gYellow<float>;
inline const math::hdr::Rgba_f gColorItemSelected = math::hdr::gGreen<float>;

class MenuScene : public Scene
{
public:
    MenuScene(const std::string & aName,
              ent::EntityManager & aWorld,
              EntityWrap<component::MappingContext> & aContext,
              ent::Handle<ent::Entity> aSceneRoot) :
        Scene(aName, aWorld, aContext, aSceneRoot), mSlots{mWorld}, mItems{mWorld}
    {}

    std::optional<Transition>
    update(GameContext & aContext, float aDelta, RawInput & aInput) override;

    void setup(GameContext & aContext,
               const Transition & aTransition,
               RawInput & aInput) override;

    void teardown(GameContext & aContext, RawInput & aInput) override;

private:
    ent::Query<component::PlayerSlot> mSlots;
    ent::Query<component::MenuItem, component::Text> mItems;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
