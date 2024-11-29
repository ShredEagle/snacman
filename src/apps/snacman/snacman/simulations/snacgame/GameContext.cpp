#include "GameContext.h"


#include "component/PlayerSlot.h"
#include "component/PlayerGameData.h"
#include "entity/Entity.h"
#include "graphics/AppInterface.h"
#include "snacman/simulations/snacgame/Entities.h"
#include <snac-renderer-V1/Camera.h>

#include "system/SceneStack.h"

namespace ad {
namespace snacgame {

GameContext::GameContext(snac::Resources aResources, snac::RenderThread<Renderer_t> & aRenderThread, const graphics::AppInterface & aAppInterface) :
    mResources{std::move(aResources)},
    mRenderThread{aRenderThread},
    mSceneStack{mWorld, "scene stack"},
    mSceneRoot{mWorld.addEntity("SceneRoot")},
    mSlotManager(this),
    mAppInterface{aAppInterface},
    mRandom{(uint64_t)snac::Clock::now().time_since_epoch().count(), 1}
{
    ent::Phase addSceneNodeToSceneRoot;
    ent::Entity sceneRoot = *mSceneRoot.get(addSceneNodeToSceneRoot);
    addGeoNode(*this, sceneRoot);
};

}
}
