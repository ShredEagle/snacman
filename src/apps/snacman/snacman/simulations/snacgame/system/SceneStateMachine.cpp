#include "SceneStateMachine.h"

#include "../GameContext.h"

#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/PlayerSlot.h"
#include "../component/PlayerHud.h"
#include "../component/PlayerRoundData.h"
#include "../component/Controller.h"
#include "../component/PathToOnGrid.h"
#include "../component/MenuItem.h"
#include "../component/Text.h"
#include "../component/Tags.h"
#include "../component/PlayerGameData.h"

#include "../scene/GameScene.h"
#include "../scene/MenuScene.h"
#include "../scene/SettingsScene.h"

#include <entity/EntityManager.h>
#include <entity/Entity.h>

#include <platform/Filesystem.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <memory>
#include <fstream>

using json = nlohmann::json;

namespace ad {
namespace snacgame {
namespace system {

using namespace ad::snacgame::scene;

// TODO: (franz) This is definitely not ideal
// There is no current benefit to have the scene defined in
// a file we should probably defer the creation of the different scenes
// to SnacGame.cpp
SceneStateMachine::SceneStateMachine(ent::EntityManager & aWorld,
                                     const filesystem::path & aPath,
                                     EntityWrap<component::MappingContext> & aContext,
                                     GameContext & aGameContext)
{
    std::ifstream sceneStream(aPath);

    json data = json::parse(sceneStream);

    ent::Handle<ent::Entity> sceneRoot = aWorld.addEntity();
    {
        ent::Phase createRoot;
        sceneRoot.get(createRoot)->add(component::SceneNode{});
        sceneRoot.get(createRoot)->add(component::Geometry{});
        sceneRoot.get(createRoot)->add(component::GlobalPose{});
    }
    aGameContext.mRoot = sceneRoot;

    for (auto sceneDesc : data)
    {
        std::string name = sceneDesc["name"];
        if (name == gMainSceneName)
        {
            mPossibleScene.push_back(
                std::make_shared<MenuScene>(name, aGameContext, aContext, sceneRoot));
        }
        else if (name == gSettingsSceneName)
        {
            mPossibleScene.push_back(
                std::make_shared<SettingsScene>(name, aGameContext, aContext, sceneRoot));
        }
        else if (name == gGameSceneName)
        {
            mPossibleScene.push_back(
                std::make_shared<GameScene>(name, aGameContext, aContext, sceneRoot));
        }
    }

    for (auto & sceneDesc : data)
    {
        auto sceneDataIt =
            std::find_if(mPossibleScene.begin(), mPossibleScene.end(),
                         [&sceneDesc](std::shared_ptr<Scene> & d) {
                             return d->mName == sceneDesc["name"].get<std::string>();
                         });
        assert(sceneDataIt != mPossibleScene.end() && "Cannot find scene");

        Scene * sceneData = sceneDataIt->get();

        for (auto & transition : sceneDesc["transition"])
        {
            auto targetIt =
                std::find_if(mPossibleScene.begin(), mPossibleScene.end(),
                             [&transition](std::shared_ptr<Scene> & d) {
                                 return d->mName == transition["target"].get<std::string>();
                             });

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
        newScene->setup(aTransition, aInput);
    }
}

scene::Scene * SceneStateMachine::getCurrentScene() const
{
    return mPossibleScene.at(mCurrentSceneId.mIndexInPossibleState).get();
}

} // namespace system
} // namespace snacgame
} // namespace ad
