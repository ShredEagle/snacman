#pragma once


#include "Renderer.h"
#include "../../RenderThread.h"

#include <entity/EntityManager.h>
#include <resource/ResourceFinder.h>


namespace ad {
namespace snacgame {


struct GameContext
{
    resource::ResourceFinder mResource;
    ent::EntityManager & mWorld;
    snac::RenderThread<Renderer> & mRenderThread;
};


} // namespace snacgame
} // namespace ad
