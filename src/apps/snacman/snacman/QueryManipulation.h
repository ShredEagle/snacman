
#include <entity/Entity.h>
#include <entity/Query.h>
#include <handy/FunctionTraits.h>

namespace ad {
namespace snac {

template <class T_predicate, std::size_t... I, class... VT_components>
ent::Handle<ent::Entity>
getFirstHandle_impl(ent::Query<VT_components...> aQuery,
                    T_predicate aLambda,
                    std::index_sequence<I...>)
{
    ent::Handle<ent::Entity> resultHandle;

    aQuery.each([&resultHandle, &aLambda](
                    ent::Handle<ent::Entity> aHandle,
                    handy::FunctionArgument_t<T_predicate, I>... parameters) {
        if (aLambda(parameters...) && !resultHandle.isValid())
        {
            resultHandle = aHandle;
        }
    });

    return resultHandle;
}

template <class T_predicate,
          class T_parameters_size =
              std::make_index_sequence<ad::handy::FunctionArity_v<T_predicate>>,
          class... VT_components>
ent::Handle<ent::Entity>
getFirstHandle(ent::Query<VT_components...> aQuery, T_predicate aLambda)
{
    return getFirstHandle_impl(aQuery, aLambda, T_parameters_size{});
}

template <class... VT_components>
ent::Handle<ent::Entity>
getFirstHandle(ent::Query<VT_components...> aQuery)
{
    return getFirstHandle(aQuery, []() { return true; });
}

template <class... VT_components>
void
eraseAllInQuery(ent::Query<VT_components...> aQuery)
{
    ent::Phase destroy;
    aQuery.each([&destroy](ent::Handle<ent::Entity> aHandle, VT_components... parameters) {
        aHandle.get(destroy)->erase();
    });
}

} // namespace snac
} // namespace ad
