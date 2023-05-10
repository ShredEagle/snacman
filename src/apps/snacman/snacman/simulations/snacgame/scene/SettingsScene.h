#pragma once

#include "Scene.h"

namespace ad {

struct RawInput;

namespace snacgame {

struct GameContext;

namespace scene {

class SettingsScene : public Scene
{
public:
    using Scene::Scene;

    std::optional<Transition> update(const snac::Time & aTime,
                                     RawInput & aInput) override;
    void setup(const Transition & Transition, RawInput & aInput) override;
    void teardown(RawInput & aInput) override;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
