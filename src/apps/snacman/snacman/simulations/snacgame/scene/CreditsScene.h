#pragma once

#include "Scene.h"

#include <snacman/Timing.h>

#include <entity/Wrap.h>


namespace ad {
namespace snacgame {
namespace scene {

class CreditsScene : public Scene
{
public:
    CreditsScene(GameContext & aGameContext,
                 ent::Wrap<component::MappingContext> & aContext);

    void update(const snac::Time & aTime, RawInput & aInput) override;

    void onEnter(Transition aTransition) override;

    void onExit(Transition aTransition) override;

private:
    void populateSection(std::size_t aSectionIdx);
    void setOpacity(float aOpacity);

    void clearEntities(ent::Phase & aPhase);

    snac::Clock::duration mTimeAccu;
    std::size_t mCurrentSection = 0;

    ent::Handle<ent::Entity> mSectionTitle;
    std::vector<ent::Handle<ent::Entity>> mNames;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
