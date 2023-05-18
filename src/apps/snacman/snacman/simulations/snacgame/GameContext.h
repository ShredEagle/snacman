#pragma once


#include "Renderer.h"
#include "SimulationControl.h"
#include "../../RenderThread.h"
#include "../../Resources.h"

#include <entity/EntityManager.h>
#include <resource/ResourceFinder.h>

#include <snac-renderer/DebugDrawer.h>


namespace ad {
namespace snacgame {


struct GameContext
{
    snac::Resources mResources;
    ent::EntityManager mWorld;
    snac::RenderThread<Renderer> & mRenderThread;
    SimulationControl mSimulationControl;
    std::optional<ent::Handle<ent::Entity>> mLevel;
    std::optional<ent::Handle<ent::Entity>> mRoot; // only because it cannot be assigned on construction, it should not change
};


} // namespace snacgame
} // namespace ad
