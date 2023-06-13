#pragma once

#include "../scene/Scene.h"

#include <string>

namespace std {
namespace filesystem {
class path;
}
}

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
                      const std::filesystem::path & aPath,
                      EntityWrap<component::MappingContext> & aContext,
                      GameContext & aGameContext);
    void changeState(GameContext & aContext, scene::Transition & aTransition, RawInput & aInput);
    scene::Scene * getCurrentScene() const;

private:
    std::vector<std::shared_ptr<scene::Scene>> mPossibleScene;
    scene::SceneId mCurrentSceneId{0};
};

} // namespace system
} // namespace snacgame
} // namespace ad
