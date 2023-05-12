#pragma once

#include "Scene.h"

#include "../component/Controller.h"
#include "../component/LevelTags.h"
#include "../component/PathToOnGrid.h"
#include "../component/PlayerHud.h"
#include "../component/PlayerSlot.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>

#include <string>

namespace ad {

struct RawInput;

namespace snacgame {

struct GameContext;
template <class T_wrapped> struct EntityWrap;

namespace component {
struct MappingContext;
}

namespace scene {

class GameScene : public Scene
{
public:
    GameScene(const std::string & aName,
            GameContext & aGameContext,
              EntityWrap<component::MappingContext> & aContext,
              ent::Handle<ent::Entity> aSceneRoot
              );

    std::optional<Transition> update(const snac::Time & aTime,
                                     RawInput & aInput) override;

    void setup(const Transition & aTransition, RawInput & aInput) override;

    void teardown(RawInput & aInput) override;

private:
    ent::Query<component::LevelEntity> mTiles;
    ent::Query<component::PlayerSlot> mSlots;
    ent::Query<component::PlayerSlot, component::Controller> mPlayers;
    ent::Query<component::PlayerHud> mHuds;
    ent::Query<component::PathToOnGrid> mPathfinders;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
