#pragma once

#include "snacman/simulations/snacgame/scene/Scene.h"

#include <algorithm>
#include <entity/EntityManager.h>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <platform/Filesystem.h>

namespace ad {
namespace snacgame {
namespace system {

const inline std::string gMainSceneName = "Main";
const inline std::string gSettingsSceneName = "Settings";
const inline std::string gGameSceneName = "Game";

class SceneStateMachine
{
public:
    SceneStateMachine(ent::EntityManager & aWorld,
                      const filesystem::path & aPath,
                      EntityWrap<component::Context> & aContext);
    void changeState(scene::Transition & aTransition, RawInput & aInput);
    scene::Scene * getCurrentScene() const;

private:
    std::vector<std::shared_ptr<scene::Scene>> mPossibleScene;
    scene::SceneId mCurrentSceneId{0};
};

} // namespace system
} // namespace snacgame
} // namespace ad
