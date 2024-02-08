#include "SceneStack.h"

#include <snac-renderer-V1/Camera.h>

namespace ad {
namespace snacgame {
namespace system {

using namespace ad::snacgame::scene;

// TODO: (franz) This is definitely not ideal
// There is no current benefit to have the scene defined in
// a file we should probably defer the creation of the different scenes
// to SnacGame.cpp

void SceneStack::pushScene(std::shared_ptr<Scene> && aScene,
                           Transition aTransition)
{
    if (!mSceneStack.empty())
    {
        mSceneStack.back()->onExit(aTransition);
    }
    mSceneStack.push_back(std::move(aScene));
    mSceneStack.back()->onEnter(aTransition);
}

bool SceneStack::empty()
{
    return mSceneStack.empty();
}

void SceneStack::popScene(Transition aTransition)
{
    assert(!mSceneStack.empty());

    Scene * scene = mSceneStack.back().get();
    scene->onExit(aTransition);
    mSceneStack.pop_back();
    if (!mSceneStack.empty())
    {
        mSceneStack.back()->onEnter(aTransition);
    }
}

Scene * SceneStack::getActiveScene() const
{
    assert(!mSceneStack.empty());
    return (Scene *)mSceneStack.back().get();
}

} // namespace system
} // namespace snacgame
} // namespace ad
