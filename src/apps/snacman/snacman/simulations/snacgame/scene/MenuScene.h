#pragma once

#include "Scene.h"

#include <snacman/Timing.h>
#include <snacman/Input.h>

#include <math/Color.h>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace component {
struct MenuItem;
struct Text;
}
namespace scene {

inline const math::hdr::Rgba_f gColorItemUnselected = math::hdr::gBlack<float>;
inline const math::hdr::Rgba_f gColorItemSelected = math::hdr::gCyan<float>;

class MenuScene : public Scene
{
public:
    MenuScene(std::string aName,
            GameContext & aGameContext,
              EntityWrap<component::MappingContext> & aContext,
              ent::Handle<ent::Entity> aSceneRoot);

    std::optional<Transition> update(const snac::Time & aTime, RawInput & aInput) override;

    void setup(const Transition & aTransition,
               RawInput & aInput) override;

    void teardown(RawInput & aInput) override;

private:
    ent::Query<component::MenuItem, component::Text> mItems;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
