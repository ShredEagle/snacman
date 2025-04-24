#pragma once

#include "entity/Entity.h"
#include "../scene/Scene.h"

namespace ad {
namespace snacgame {
namespace system {

struct SceneStack
{
public:
    scene::Scene * getActiveScene() const;

    void pushScene(std::shared_ptr<scene::Scene> && aScene,
                   scene::Transition aTransition = {});
    void popScene(scene::Transition aTransition = {});
    bool empty();

private:
    // Shared pointer because all component or system needs to be copyable
    // because save state
    std::vector<std::shared_ptr<scene::Scene>> mSceneStack;
};

} // namespace system
} // namespace snacgame
} // namespace ad
