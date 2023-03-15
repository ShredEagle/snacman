#pragma once

#include "Scene.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>
#include <string>

namespace ad {

struct RawInput;

namespace snacgame {

struct GameContext;
template <class T_wrapped> struct EntityWrap;

namespace component {
struct Controller;
struct LevelEntity;
struct MappingContext;
struct PlayerSlot;
}

namespace scene {

class GameScene : public Scene
{
public:
    GameScene(const std::string & aName,
              ent::EntityManager & aWorld,
              EntityWrap<component::MappingContext> & aContext);

    std::optional<Transition> update(GameContext & aContext,
                                     float aDelta,
                                     RawInput & aInput) override;

    void setup(GameContext & aContext, const Transition & aTransition, RawInput & aInput) override;

    void teardown(RawInput & aInput) override;

private:
    ent::Handle<ent::Entity> mLevel;
    ent::Query<component::LevelEntity> mTiles;
    ent::Query<component::PlayerSlot> mSlots;
    ent::Query<component::PlayerSlot, component::Controller> mPlayers;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
