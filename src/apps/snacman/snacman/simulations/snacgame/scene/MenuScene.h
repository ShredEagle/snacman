#pragma once

#include "Scene.h"

#include "../component/MenuItem.h"
#include "../component/PlayerSlot.h"
#include "../component/Text.h"

#include <snacman/Input.h>

#include <math/Color.h>
#include <entity/Query.h>
#include <string>

namespace ad {
namespace snacgame {
namespace scene {

inline const math::hdr::Rgba_f gColorItemUnselected = math::hdr::gBlack<float>;
inline const math::hdr::Rgba_f gColorItemSelected = math::hdr::gCyan<float>;

class MenuScene : public Scene
{
public:
    MenuScene(const std::string & aName,
            GameContext & aGameContext,
              EntityWrap<component::MappingContext> & aContext,
              ent::Handle<ent::Entity> aSceneRoot) :
        Scene(aName, aGameContext, aContext, aSceneRoot), mSlots{mGameContext.mWorld}, mItems{mGameContext.mWorld}
    {}

    std::optional<Transition> update(const snac::Time & aTime, RawInput & aInput) override;

    void setup(const Transition & aTransition,
               RawInput & aInput) override;

    void teardown(RawInput & aInput) override;

private:
    ent::Query<component::PlayerSlot> mSlots;
    ent::Query<component::MenuItem, component::Text> mItems;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
