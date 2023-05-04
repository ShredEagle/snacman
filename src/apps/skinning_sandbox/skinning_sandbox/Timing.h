#pragma once


#include <chrono>


namespace ad {
namespace snac {


using Clock = std::chrono::high_resolution_clock;

using ms = std::chrono::milliseconds;


constexpr double asSeconds(Clock::duration aDuration)
{
    constexpr double gTickPeriod{double(Clock::period::num) / Clock::period::den};
    return aDuration.count() * gTickPeriod;
}


} // namespace snac
} // namespace ad
