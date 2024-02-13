#pragma once

#include "GameParameters.h"
#include "SimulationControl.h"
#include "PlayerSlotManager.h"

#include "component/PlayerGameData.h"
#include "component/PlayerSlot.h"

#include "handy/Guard.h"
#include "snacman/EntityUtilities.h"
#include "system/SceneStack.h"

#include <entity/Entity.h>
#include <entity/Query.h>
#include <entity/EntityManager.h>
#include <entity/Wrap.h>

#include <resource/ResourceFinder.h>
#include <snacman/Resources.h>
#include <snac-renderer-V1/Camera.h>

namespace ad {

namespace snac {
template <class T>
class RenderThread;
}

namespace snacgame {

namespace component {
struct LevelSetupData;
} // namespace component

class Renderer;

struct GameContext
{
    GameContext(snac::Resources aResources, snac::RenderThread<Renderer> & aRenderThread);

    snac::Resources mResources;
    ent::EntityManager mWorld;
    snac::RenderThread<Renderer> & mRenderThread;
    SimulationControl mSimulationControl;

    ent::Wrap<system::SceneStack> mSceneStack;
    ent::Handle<ent::Entity> mSceneRoot;
    PlayerSlotManager mSlotManager;
};

} // namespace snacgame
} // namespace ad
