#pragma once

#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

namespace nlohmann {
template <class Clock, class Duration>
struct adl_serializer<std::chrono::time_point<Clock, Duration>>
{
    static void to_json(json & aJson, const std::chrono::time_point<Clock, Duration> & aTimePoint)
    {
        aJson["since_epoch"] = aTimePoint.time_since_epoch().count();
    }

    static void from_json(const json & aJson, std::chrono::time_point<Clock, Duration> & aTimePoint)
    {
        const Duration since_epoch(static_cast<Duration::rep>(aJson["since_epoch"]));
        aTimePoint = std::chrono::time_point<Clock, Duration>(since_epoch);
    }
};
} // namespace nlohmann
