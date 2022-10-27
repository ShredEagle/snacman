#pragma once


#include "Timing.h"

#include <math/Color.h>

#include <list>
#include <mutex>
#include <optional>


namespace ad {
namespace snac {


struct GraphicState
{
    math::sdr::Rgb color;
};


template <class T_state>
class StateFifo
{
public:
    struct Entry
    {
        Clock::time_point pushTime;
        std::unique_ptr<T_state> state;
    };


    void push(std::unique_ptr<T_state> aState)
    {
        std::lock_guard lock{mMutex};
        mStore.push_back({Clock::now(), std::move(aState)});
    }


    Entry pop()
    {
        std::lock_guard lock{mMutex};
        Entry result = std::move(mStore.front());
        mStore.pop_front();
        return result;
    }

    bool empty()
    {
        std::lock_guard lock{mMutex};
        return mStore.empty();
    }


    std::optional<Entry> popToEnd()
    {
        std::optional<Entry> result;
        std::lock_guard lock{mMutex};
        if (! mStore.empty())
        {
            result = std::move(mStore.back());
            mStore.clear();
        }
        return result;
    }


private:
    std::mutex mMutex;
    std::list<Entry> mStore;
};


} // namespace snac
} // namespace ad
