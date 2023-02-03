#include "SceneStateMachine.h"

#include "../scene/GameScene.h"
#include "../scene/MenuScene.h"
#include "../scene/SettingsScene.h"

#include <entity/Entity.h>

using json = nlohmann::json;

namespace ad {
namespace snacgame {
namespace system {

using namespace ad::snacgame::scene;

SceneStateMachine::SceneStateMachine(ent::EntityManager & aWorld,
                                     const filesystem::path & aPath,
                                     EntityWrap<component::MappingContext> & aContext)
{
    std::ifstream sceneStream(aPath);

    json data = json::parse(sceneStream);

    for (auto sceneDesc : data)
    {
        std::string name = sceneDesc["name"];
        if (name.compare(gMainSceneName) == 0)
        {
            mPossibleScene.push_back(
                std::make_shared<MenuScene>(name, aWorld, aContext));
        }
        else if (name.compare(gSettingsSceneName) == 0)
        {
            mPossibleScene.push_back(
                std::make_shared<SettingsScene>(name, aWorld, aContext));
        }
        else if (name.compare(gGameSceneName) == 0)
        {
            mPossibleScene.push_back(
                std::make_shared<GameScene>(name, aWorld, aContext));
        }
    }

    for (auto & sceneDesc : data)
    {
        auto sceneDataIt =
            std::find_if(mPossibleScene.begin(), mPossibleScene.end(),
                         [&sceneDesc](std::shared_ptr<Scene> & d) {
                             return d->mName == sceneDesc["name"];
                         });
        assert(sceneDataIt != mPossibleScene.end() && "Cannot find scene");

        Scene * sceneData = sceneDataIt->get();

        for (auto & transition : sceneDesc["transition"])
        {
            auto targetIt =
                std::find_if(mPossibleScene.begin(), mPossibleScene.end(),
                             [&transition](std::shared_ptr<Scene> & d) {
                                 return d->mName == transition["target"];
                             });

            [[maybe_unused]] const auto [it, success] =
                sceneData->mStateTransition.insert(
                    {Transition{transition["transition"]},
                     {static_cast<std::size_t>(
                         std::distance(mPossibleScene.begin(), targetIt))}});
        }
    }
}

void SceneStateMachine::changeState(GameContext & aContext, Transition & aTransition, RawInput & aInput)
{
    Scene * oldScene =
        mPossibleScene.at(mCurrentSceneId.mIndexInPossibleState).get();
    assert(oldScene->mStateTransition.contains(aTransition)
           && "Current state does not contain the transition");

    if (aTransition.shouldTeardown)
    {
        oldScene->teardown(aInput);
    }

    mCurrentSceneId = oldScene->mStateTransition.at(aTransition);

    Scene * newScene =
        mPossibleScene.at(mCurrentSceneId.mIndexInPossibleState).get();

    if (aTransition.shouldSetup)
    {
        newScene->setup(aContext, aTransition, aInput);
    }
}

scene::Scene * SceneStateMachine::getCurrentScene() const
{
    return mPossibleScene.at(mCurrentSceneId.mIndexInPossibleState).get();
}

} // namespace system
} // namespace snacgame
} // namespace ad
