#pragma once

#include <entity/Entity.h>
#include <handy/StringId.h>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <unordered_map>
#include <utility>

namespace ad {
namespace snacgame {
namespace detail {

using json = nlohmann::json;

class TypedErasedConstructor;

template <class T_type>
inline std::shared_ptr<TypedErasedConstructor>
addConstructor(handy::StringId aId);

inline std::unordered_map<handy::StringId,
                          std::shared_ptr<TypedErasedConstructor>> &
handlebinderInstance()
{
    static std::unordered_map<handy::StringId,
                              std::shared_ptr<TypedErasedConstructor>>
        handleBinderStore = {};
    return handleBinderStore;
}

class TypedErasedConstructor
{
public:
    TypedErasedConstructor() = default;
    TypedErasedConstructor(const TypedErasedConstructor &) = default;
    TypedErasedConstructor(TypedErasedConstructor &&) = delete;
    TypedErasedConstructor &
    operator=(const TypedErasedConstructor &) = default;
    TypedErasedConstructor & operator=(TypedErasedConstructor &&) = delete;
    virtual ~TypedErasedConstructor() = default;
    virtual void construct(ent::Handle<ent::Entity> aHandle,
                           const json & aData) = 0;
};

template <class T_type>
class TypedConstructor : public TypedErasedConstructor
{
public:
    static std::shared_ptr<TypedConstructor<T_type>> bind(const char * aName)
    {
        return std::dynamic_pointer_cast<TypedConstructor<T_type>>(
            addConstructor<T_type>(handy::StringId(aName)));
    }

    void construct(ent::Handle<ent::Entity> aHandle,
                   const json & aData) override;
};

template <class T_type>
std::shared_ptr<TypedErasedConstructor> addConstructor(handy::StringId aId)
{
    auto constructor = std::make_shared<TypedConstructor<T_type>>();
    handlebinderInstance().insert_or_assign(aId, constructor);
    return constructor;
}

#define SNAC_SERIAL_REGISTER(name)                                             \
    struct InitConstructor##name                                               \
    {                                                                          \
        static inline const std::shared_ptr<                                   \
            ::ad::snacgame::detail::TypedConstructor<name>>                    \
            a = ::ad::snacgame::detail::TypedConstructor<name>::bind(#name);   \
    };
} // namespace detail
} // namespace snacgame
} // namespace ad
