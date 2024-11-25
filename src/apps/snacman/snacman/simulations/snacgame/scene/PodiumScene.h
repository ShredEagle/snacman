#pragma once

#include "Scene.h"

#include "../component/Text.h"
#include "../component/MenuItem.h"

#include <math/Color.h>
#include <entity/Query.h>
#include <entity/Wrap.h>

namespace ad {
namespace snacgame {
namespace scene {

class PodiumScene : public Scene
{
public:
    PodiumScene(GameContext & aGameContext,
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
