#pragma once


#include <handy/Guard.h>

#include <initializer_list>
#include <map>


namespace ad {
namespace snac {


// TODO It might actually have been better to just "copy" a plain map each time we enter a scope were modification are to be made.
// The guard to restore the previous values is not trivial, and requires several heap allocations.
template <class T_key, class T_value>
struct StackRepository
{
    using Store_t = std::map<T_key, T_value>;

    StackRepository(std::initializer_list<typename Store_t::value_type> aInit) :
        mStore{aInit}
    {}

    template <class... VT_ctorArgs>
    StackRepository(VT_ctorArgs &&... aCtorArgs) :
        mStore{std::forward<VT_ctorArgs>(aCtorArgs)...}
    {}

    auto find(const T_key & aKey) const
    { return mStore.find(aKey); }

    auto begin() const
    { return mStore.begin(); }
    auto end() const
    { return mStore.end(); }

    Guard push(T_key aKey, T_value aValue)
    {
        return push(StackRepository{{aKey, std::move(aValue)}});
    }

    Guard push(const StackRepository & aOther)
    {
        std::vector<T_key> toRemove;
        std::vector<std::pair<T_key, T_value>> toRestore;

        for(auto [key, value] : aOther)
        {
            auto [iterator, didInsert] = mStore.try_emplace(key, std::move(value));
            if (didInsert)
            {
                toRemove.push_back(key);
            }
            else
            {
                toRestore.push_back({key, std::move(iterator->second)});
                iterator->second = std::move(value);
            }
        }

        return Guard([toRemove = std::move(toRemove), toRestore = std::move(toRestore), this]
        {
            for (auto key : toRemove)
            {
                this->mStore.erase(key);
            }
            for (auto it = toRestore.begin(); it != toRestore.end(); ++it)
            {
                this->mStore.at(it->first) = std::move(it->second);
            }
        });
    }

    Store_t mStore;
};


} // namespace snac
} // namespace ad