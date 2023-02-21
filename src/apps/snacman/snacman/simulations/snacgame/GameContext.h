#pragma once


#include "Renderer.h"
#include "../../RenderThread.h"
#include "../../Resources.h"

#include <entity/EntityManager.h>
#include <resource/ResourceFinder.h>


namespace ad {
namespace snacgame {


struct GameContext
{
    snac::Resources mResources; // contains Freetype, which must outlive the entity manager
    ent::EntityManager mWorld;
    snac::RenderThread<Renderer> & mRenderThread;
};


} // namespace snacgame
} // namespace ad
