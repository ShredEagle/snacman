#pragma once


#include <concepts>


namespace ad {


template <std::integral T_value>
struct Clamped
{
    T_value & mValue;
    T_value mMin;
    T_value mMax;
};


} // namespace ad