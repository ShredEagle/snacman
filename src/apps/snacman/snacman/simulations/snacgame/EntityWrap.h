#pragma once

#include <entity/EntityManager.h>

namespace ad {
namespace snacgame {

// TODO re-implement in term of query? Might be much easier.
// But how to address the situation where there would be several EntityWrap for the same type?

// TODO Should be extended with a phaseless get (to be implemented), as arrow operator.
// becasue, by nature, this should not allow to change Archetype or remove the entity.

// Note: one drawback is that we make one phase per initialized handle.
// When doing it manually, we would share one init phase.
// Note: currently, mWrapped member is mutable, see comment below. 
template <class T_wrapped>
struct EntityWrap
{
    template <class... VT_ctorArgs>
    EntityWrap(ent::EntityManager & aWorld, VT_ctorArgs &&... aCtorArgs) :
        mWrapped{aWorld.addEntity()}
    {
        ent::Phase init;
        // TODO emplace when entity offers it
        // TODO would be nice to automatically forward world when it is the first ctor argument
        mWrapped.get(init)->add(T_wrapped{/*aWorld,*/ std::forward<VT_ctorArgs>(aCtorArgs)...});
    }

    T_wrapped & get(ent::Phase & aPhase) const
    {
        auto entity = mWrapped.get(aPhase);
        assert(entity);
        return entity->get<T_wrapped>();
    }

    T_wrapped & operator*() const
    {
        ent::Phase dummy;
        return get(dummy);
    };

    T_wrapped * operator->() const
    {
        return &(**this);
    }
        
    // Made mutable because there is no overload const overload of Handle<>::get()
    mutable ent::Handle<ent::Entity> mWrapped;
};


} // namespace cubes
} // namespace ad
