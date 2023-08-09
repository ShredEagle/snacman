#pragma once


#include <chrono>


namespace ad {


using Clock = std::chrono::high_resolution_clock;


// TODO should we constrain it to Clock::duration?
/// @brief Returns an **integral** number of ticks.
template <class T_targetDuration, class T_durationRep, class T_durationPeriod>
constexpr auto getTicks(std::chrono::duration<T_durationRep, T_durationPeriod> aDuration)
{
    return std::chrono::duration_cast<T_targetDuration>(aDuration).count();
}


constexpr double asFractionalSeconds(Clock::duration aDuration)
{
    constexpr double gTickPeriod{double(Clock::period::num) / Clock::period::den};
    return aDuration.count() * gTickPeriod;
}

} // namespace ad