#include "GameContext.h"

#include "component/PlayerSlot.h"
#include "component/PlayerGameData.h"
#include "entity/Entity.h"
#include "snacman/simulations/snacgame/Entities.h"
#include <snac-renderer-V1/Camera.h>

namespace ad {
namespace snacgame {

GameContext::GameContext(snac::Resources aResources, snac::RenderThread<Renderer_t> & aRenderThread) :
    mResources{aResources},
    mRenderThread{aRenderThread},
    mSceneStack{mWorld},
    mSceneRoot{mWorld.addEntity()},
    mSlotManager(this)
{
    ent::Phase addSceneNodeToSceneRoot;
    ent::Entity sceneRoot = *mSceneRoot.get(addSceneNodeToSceneRoot);
    addGeoNode(*this, sceneRoot);
};

}
}
