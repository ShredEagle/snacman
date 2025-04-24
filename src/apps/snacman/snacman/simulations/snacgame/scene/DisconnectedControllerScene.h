#pragma once

#include "Scene.h"

#include "../system/SceneStack.h"

#include <math/Color.h>
#include <entity/Query.h>
#include <entity/Wrap.h>

namespace ad {

namespace snacgame {
namespace component {
struct MenuItem;
struct Text;
struct PlayerSlot;
struct PlayerGameData;
}
namespace scene {

class DisconnectedControllerScene : public Scene
{
public:
    DisconnectedControllerScene(GameContext & aGameContext,
              ent::Wrap<component::MappingContext> & aContext);

    void update(const snac::Time & aTime, RawInput & aInput) override;

    void onEnter(Transition aTransition) override;

    void onExit(Transition aTransition) override;

    static constexpr char sKickTransitionName[] = "sKickTransition";
private:
    ent::Query<component::MenuItem, component::Text> mItems;
    ent::Query<component::PlayerSlot, component::PlayerGameData> mSlots;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
