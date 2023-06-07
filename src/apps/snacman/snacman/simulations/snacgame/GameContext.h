#pragma once

#include "EntityWrap.h"
#include "GameParameters.h"
#include "SimulationControl.h"
#include "PlayerSlotManager.h"

#include <entity/Entity.h>
#include <entity/Query.h>
#include <entity/EntityManager.h>

#include <resource/ResourceFinder.h>
#include <snacman/Resources.h>

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
    GameContext(snac::Resources aResources, snac::RenderThread<Renderer> & aRenderThread) :
        mResources{aResources},
        mRenderThread{aRenderThread},
        mSlotManager(this)
    {};

    snac::Resources mResources;
    ent::EntityManager mWorld;
    snac::RenderThread<Renderer> & mRenderThread;
    SimulationControl mSimulationControl;

    // The level entity is not stored anymore between rounds
    // so we need something else to store the level data
    // (markov file, asset root, seed) which is not really
    // not data that the Level should own
    std::optional<EntityWrap<component::LevelSetupData>> mLevelData;
    std::optional<ent::Handle<ent::Entity>> mLevel;
    std::optional<ent::Handle<ent::Entity>>
        mRoot; // only because it cannot be assigned on construction, it should
               // not change
    PlayerSlotManager mSlotManager;
};

} // namespace snacgame
} // namespace ad
