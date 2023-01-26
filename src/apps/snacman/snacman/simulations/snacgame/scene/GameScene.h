#pragma once

#include "Scene.h"
#include "snacman/simulations/snacgame/EntityWrap.h"

#include <entity/EntityManager.h>
#include <string>

namespace ad {
namespace snacgame {
namespace scene {

class GameScene : public Scene
{
public:
    GameScene(const std::string & aName,
              ent::EntityManager & aWorld,
              EntityWrap<component::Context> & aContext) :
        Scene(aName, aWorld, aContext), mLevel{mWorld.addEntity()}
    {}

    std::optional<Transition> update(float aDelta,
                                     const RawInput & aInput) override;
    void setup(const Transition & aTransition) override;
    void teardown() override;

private:
    ent::Handle<ent::Entity> mLevel;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
