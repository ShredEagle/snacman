#pragma once

#include "Timing.h"

namespace ad {
namespace snac {

constexpr Clock::duration gSimulationDelta = Clock::duration{std::chrono::seconds{1}} / 60;

struct ConfigurableSettings
{
    Clock::duration mSimulationDelta{
        gSimulationDelta};
    Clock::duration mUpdateDuration{0};
    std::atomic<bool> mInterpolate{true};
};
} // namespace snac
} // namespace ad
