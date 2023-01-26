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

    std::optional<Transition> update(float aDelta,
                                     const RawInput & aInput) override;
    void setup(const Transition & Transition) override;
    void teardown() override;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
