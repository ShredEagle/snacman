#pragma once

#include "Serialization.h"

#include <entity/Entity.h>
#include <handy/StringId.h>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace reflexion {

using json = nlohmann::json;

class TypedErasedConstructor;

template <class T_type>
inline std::shared_ptr<TypedErasedConstructor>
addConstructor(ad::handy::StringId aId);

inline std::unordered_map<std::type_index,
                          std::shared_ptr<TypedErasedConstructor>> &
indexedTypedConstrutorInstance()
{
    static std::unordered_map<std::type_index,
                              std::shared_ptr<TypedErasedConstructor>>
        indexedTypedConstructorStore = {};
    return indexedTypedConstructorStore;
}

inline std::unordered_map<std::string,
                          std::type_index> &
nameTypeIndexInstance()
{
    static std::unordered_map<std::string,
                              std::type_index>
        nameTypeIndexStore = {};
    return nameTypeIndexStore;
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
    virtual void construct(ad::ent::Handle<ad::ent::Entity> aHandle,
                           const json & aData) = 0;
    virtual void serialize(json & aJson,
                           ad::ent::Handle<ad::ent::Entity> aHandle) = 0;
};

template <class T_type>
class TypedConstructor : public TypedErasedConstructor
{
public:
    TypedConstructor(const char * aName) :
        mName{aName}
    {
        
    }

    static std::shared_ptr<TypedConstructor<T_type>> bind(const char * aName);

    void construct(ad::ent::Handle<ad::ent::Entity> aHandle,
                   const json & aData) override;

    void serialize(json & aData,
                   ad::ent::Handle<ad::ent::Entity> aHandle) override;

    const char * mName;
};

template <class T_type>
std::shared_ptr<TypedErasedConstructor> addConstructor(const char * aName)
{
    auto constructor = std::make_shared<TypedConstructor<T_type>>(aName);
    indexedTypedConstrutorInstance().insert_or_assign(std::type_index(typeid(T_type)), constructor);
    nameTypeIndexInstance().insert_or_assign(aName, std::type_index(typeid(T_type)));
    return constructor;
}

template <class T_type>
std::shared_ptr<TypedConstructor<T_type>> TypedConstructor<T_type>::bind(const char * aName)
{
    return std::dynamic_pointer_cast<TypedConstructor<T_type>>(
        addConstructor<T_type>(aName));
}

#define SNAC_SERIAL_REGISTER(name)                                             \
    struct InitConstructor##name                                               \
    {                                                                          \
        static inline const std::shared_ptr<reflexion::TypedConstructor<name>> \
            a = reflexion::TypedConstructor<name>::bind(#name);                \
    };
} // namespace reflexion
