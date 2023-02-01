#pragma once

#include "Scene.h"

#include <string>

namespace ad {
namespace snacgame {
namespace scene {

class SettingsScene : public Scene
{
public:
    using Scene::Scene;

    std::optional<Transition> update(GameContext & aContext,
                                     float aDelta,
                                     RawInput & aInput) override;
    void setup(const Transition & Transition, RawInput & aInput) override;
    void teardown(RawInput & aInput) override;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
