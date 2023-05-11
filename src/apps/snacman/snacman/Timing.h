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

// Note: There are several timelines and periods involved:
// * The wall-time timepoint ("real world time")
// * The simulation update period == how much real-time should ideally elapse between two call to Simulation::update()
// * The simulation timepoint (what is the time in the simulation)
// * The simulation delta == the duration that should be simulated by the call to Simulation::update()
struct Time
{
    /// @brief Advance by the currently set delta
    Time & advance()
    {
        mTimepoint += mDeltaDuration;
        return *this;
    }

    /// @brief Advance by the newly set delta
    Time & advance(Clock::duration aDelta)
    {
        setDelta(aDelta);
        return advance();
    }

    /// @brief Set the provided timepoint, computing the delta from last timepoint
    Time & setTimepoint(Clock::time_point aTimepoint)
    {
        setDelta(aTimepoint - mTimepoint);
        mTimepoint = aTimepoint;
        return *this;
    }

private:
    void setDelta(Clock::duration aDelta)
    { 
        mDeltaDuration = aDelta;
        mDeltaSeconds = asSeconds(mDeltaDuration); 
    }

public:
    Clock::time_point mTimepoint; // the timepoint in the timeline of the simulation
    Clock::duration mDeltaDuration; // 
    double mDeltaSeconds;
};


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
