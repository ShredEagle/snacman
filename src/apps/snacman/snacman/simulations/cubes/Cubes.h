#pragma once


#include "ComponentGeometry.h"

#include "Renderer.h"

#include <entity/EntityManager.h>

#include <memory>


namespace ad {
namespace cubes {


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
        mWrapped.get(init)->add(T_wrapped{aWorld, std::forward<VT_ctorArgs>(aCtorArgs)...});
    }

    T_wrapped & get(ent::Phase & aPhase)
    {
        return mWrapped.get(aPhase)->get<T_wrapped>();
    }
        
    T_wrapped & get(ent::Phase & aPhase) const
    {
        return mWrapped.get(aPhase)->get<T_wrapped>();
    }
        
    // Made mutable because there is no overload const overload of Handle<>::get()
    mutable ent::Handle<ent::Entity> mWrapped;
};


class Cubes
{
public:
    using Renderer_t = Renderer;

    /// \brief Initialize the scene;
    Cubes();

    void update(float aDelta);

    std::unique_ptr<GraphicState> makeGraphicState() const; 

private:
    ent::EntityManager mWorld;
    ent::Handle<ent::Entity> mSystemMove;
    EntityWrap<ent::Query<component::Geometry>> mQueryRenderable;
};


} // namespace cubes
} // namespace ad
