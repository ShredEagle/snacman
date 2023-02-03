#pragma once


#include <chrono>
#include <thread>


namespace ad {
namespace snac {


using Clock = std::chrono::high_resolution_clock;

using ms = std::chrono::milliseconds;


constexpr double asSeconds(Clock::duration aDuration)
{
    constexpr double gTickPeriod{double(Clock::period::num) / Clock::period::den};
    return aDuration.count() * gTickPeriod;
}


struct SleepResult
{
    Clock::time_point timeBefore;
    Clock::duration targetDuration;
};


inline SleepResult sleepBusy(Clock::duration aDuration, Clock::time_point aSleepFrom = Clock::now())
{
    auto beforeSleep = Clock::now();
    Clock::duration ahead = aSleepFrom + aDuration - beforeSleep;
    while((aSleepFrom + aDuration) > Clock::now())
    {
        std::this_thread::sleep_for(Clock::duration{0});
    }
    return {beforeSleep, ahead};
}


} // namespace snac
} // namespace ad
