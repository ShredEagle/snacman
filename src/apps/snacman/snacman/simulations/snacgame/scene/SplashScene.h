#pragma once

#include "Scene.h"

#include <snacman/Timing.h>

#include <entity/Wrap.h>


namespace ad {
namespace snacgame {
namespace scene {

class SplashScene : public Scene
{
public:
    SplashScene(GameContext & aGameContext,
                ent::Wrap<component::MappingContext> & aContext,
                renderer::Environment & mEnvMap);

    void update(const snac::Time & aTime, RawInput & aInput) override;

    void onEnter(Transition aTransition) override;

    void onExit(Transition aTransition) override;

private:
    void clearEntities(ent::Phase & aPhase);
    void transitionToMenu();
    void setOpacity(float aOpacity);

    ent::Handle<ent::Entity> mSplashEntity;
    snac::Clock::duration mTimeAccu;
    // Hack: We disable the envmap, and restore it on exit
    // otherwise it shows on the side of the splash quad (it is resized to compensate for window format)
    renderer::Environment & mEnvMapReference;
    renderer::Environment mEnvMapToRestore;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
