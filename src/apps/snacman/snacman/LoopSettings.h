#pragma once

#include "Timing.h"

namespace ad {
namespace snac {

constexpr Clock::duration gSimulationPeriod = Clock::duration{std::chrono::seconds{1}} / 100;

struct ConfigurableSettings
{
    Clock::duration mSimulationPeriod{
        gSimulationPeriod};
    Clock::duration mUpdateDuration{0};
    std::atomic<bool> mInterpolate{true};
};
} // namespace snac
} // namespace ad
