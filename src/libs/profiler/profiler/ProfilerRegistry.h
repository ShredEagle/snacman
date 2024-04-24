#pragma once


#include "Profiler.h"

#include <handy/Guard.h>


namespace ad::renderer {


class ProfilerRegistry
{
public:
    using Key = std::string;

    /// @brief Get an existing profiler
    static Profiler & Get(const Key & aProfilerKey);

    /// @brief Add a new profiler to the registry, for key `aProfilerKey`.
    /// The lifetime of added profiler is scoped to the returned Guard.
    /// @note Because some providers within the profiler require to destory GL objects,
    /// we ask the client to explicitly scope the global profiler where the GL context is valid for the whole scope.
    static Guard ScopeNewProfiler(const Key & aProfilerKey, Profiler::Providers aProviderFeatures);
};


} // namespace ad::renderer