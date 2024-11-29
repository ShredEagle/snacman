#pragma once

#include "GameParameters.h"
#include "SimulationControl.h"
#include "PlayerSlotManager.h"

#include "graphics/AppInterface.h"
#include "handy/Guard.h"
#include "snacman/EntityUtilities.h"
#include "snacman/Timing.h"

#include <cmath>
#include <cstdint>
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

namespace system {
struct SceneStack;
}

namespace component {
struct LevelSetupData;
} // namespace component

struct pcg32_random_t
{
    uint64_t state;
    uint64_t inc;
};

// Produce a sequence of 2^64 random number
inline uint32_t snac_random(pcg32_random_t* rng)
{
    uint64_t oldstate = rng->state;
    // Advance internal state
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
    // Calculate output function (XSH RR), uses old state for max ILP
    uint32_t xorshifted = (uint32_t)((oldstate >> 18u) ^ oldstate) >> 27u;
    int32_t rot = (int32_t)(oldstate >> 59u);
    return (xorshifted >> (uint32_t)rot) | (xorshifted << (uint32_t)((-rot) & 31));
}

// Produce a random bounded integer number
inline uint32_t snac_random_bounded(pcg32_random_t* rng, uint32_t bound)
{
    return snac_random(rng) % bound;
}

// Produce a random float number between 0 and 1
inline float snac_random_float(pcg32_random_t* rng)
{
    return (float)ldexp((double)snac_random(rng), -32);
}

// Produce a random float number between 0 and bound
inline float snac_random_float_bounded(pcg32_random_t* rng, float bound)
{
    return snac_random_float(rng) * bound;
}

// Produce a random float number between center - 0.5 and center + 0.5
inline float snac_random_float_centered(pcg32_random_t* rng, float center)
{
    return snac_random_float(rng) - 0.5f + center;
}

// Produce a random float number between center - bound and center + bound
inline float snac_random_float_centered_bounded(pcg32_random_t* rng, float center, float bound)
{
    return snac_random_float(rng) * 2 * bound + center;
}

struct GameContext
{
    GameContext(snac::Resources aResources, snac::RenderThread<Renderer_t> & aRenderThread, const graphics::AppInterface & aAppInterface);

    snac::Resources mResources;
    ent::EntityManager mWorld;
    snac::RenderThread<Renderer_t> & mRenderThread;
    SimulationControl mSimulationControl;

    ent::Wrap<system::SceneStack> mSceneStack;
    ent::Handle<ent::Entity> mSceneRoot;
    PlayerSlotManager mSlotManager;
    const graphics::AppInterface & mAppInterface;
    pcg32_random_t mRandom;
};

} // namespace snacgame
} // namespace ad
