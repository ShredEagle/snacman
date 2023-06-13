#include "GameContext.h"

#include "component/PlayerSlot.h"
#include "component/PlayerGameData.h"

namespace ad {
namespace snacgame {

GameContext::GameContext(snac::Resources aResources, snac::RenderThread<Renderer> & aRenderThread) :
    mResources{aResources},
    mRenderThread{aRenderThread},
    mSlotManager(this)
{};

}
}
