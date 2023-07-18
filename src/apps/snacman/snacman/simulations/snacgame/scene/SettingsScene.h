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

    void update(const snac::Time & aTime,
                                     RawInput & aInput) override;
    void onEnter(Transition Transition) override;
    void onExit(Transition aTransition) override;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
