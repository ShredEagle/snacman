#pragma once

#include "Scene.h"
#include "snac-renderer/Camera.h"

#include <math/Color.h>
#include <entity/Query.h>
#include <entity/Wrap.h>

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
    MenuScene(GameContext & aGameContext,
              ent::Wrap<component::MappingContext> & aContext);

    void update(const snac::Time & aTime, RawInput & aInput) override;

    void onEnter(Transition aTransition) override;

    void onExit(Transition aTransition) override;

private:
    ent::Query<component::MenuItem, component::Text> mItems;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
