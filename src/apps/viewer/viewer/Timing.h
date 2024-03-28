#pragma once


namespace ad::renderer {


// TODO Ad 2024/03/07: Use an integral count of ticks?
// This could be much better for timepoints.
// (from rough calculations, chosing a tick of 10µs allows to represent > 150 years in 64bits)
struct Timing
{
    float mDeltaDuration{0};
    double mSimulationTimepoint{0};

    Timing & advance(float aDelta)
    {
        mSimulationTimepoint += aDelta;
        mDeltaDuration = aDelta;
        return *this;
    }
};


} // namespace ad::renderer