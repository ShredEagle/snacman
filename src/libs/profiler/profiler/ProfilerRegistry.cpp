#include "ProfilerRegistry.h"

#include <mutex>
#include <unordered_map>


namespace ad::renderer {


namespace {


    struct RegistryStore
    {
        std::mutex mMapMutex;
        std::unordered_map<ProfilerRegistry::Key, Profiler> mMap;
    };


    RegistryStore & getRegistryStore()
    {
        static RegistryStore gStore;
        return gStore;
    }


} // unnamed namespace


Profiler & ProfilerRegistry::Get(const Key & aProfilerKey)
{
    RegistryStore & store = getRegistryStore();
    if(auto found = store.mMap.find(aProfilerKey); found != store.mMap.end())
    {
        return found->second;
    }
    else
    {
        throw std::out_of_range{
            "ProfilerRegistry cannot find a profiler associated to '" + aProfilerKey + "'."};
    }
}


Guard ProfilerRegistry::ScopeNewProfiler(const Key & aProfilerKey, Profiler::Providers aProviderFeatures)
{
    RegistryStore & store = getRegistryStore();
    const std::lock_guard<std::mutex> lock{store.mMapMutex};
    if(store.mMap.count(aProfilerKey) != 0)
    {
        throw std::logic_error{"A profiler already exist for key '" + aProfilerKey + "'"};
    }
    store.mMap.emplace(aProfilerKey, Profiler{aProviderFeatures});

    return Guard{[aProfilerKey]()
    {
        RegistryStore & store = getRegistryStore();
        const std::lock_guard<std::mutex> lock{store.mMapMutex};
        store.mMap.erase(aProfilerKey);
    }};
}


} // namespace ad::renderer
