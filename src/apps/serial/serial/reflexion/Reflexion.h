#pragma once

#include "serial/serialization/JsonWitness.h"

#include <entity/Entity.h>
#include <handy/StringId.h>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <typeindex>
#include <unordered_map>

namespace reflexion {

using json = nlohmann::json;

class TypeErasedProcessor;

template <class T_type>
inline std::shared_ptr<TypeErasedProcessor>
addConstructor(ad::handy::StringId aId);


/// \brief Map of typed constructor by type_index
/// used during Handle construction/serialization to retrieve
/// a component processor that can add a component or serialize
/// a component from a handle
inline std::unordered_map<std::type_index,
                          std::shared_ptr<TypeErasedProcessor>> &
typedProcessorStore()
{
    static std::unordered_map<std::type_index,
                              std::shared_ptr<TypeErasedProcessor>>
        typedProcessorStoreInstance = {};
    return typedProcessorStoreInstance;
}

/// \brief Map of type index by name used to retrieve a component
/// type_index, during deserialization, from its name
inline std::unordered_map<std::string,
                          std::type_index> &
nameTypeIndexStore()
{
    static std::unordered_map<std::string,
                              std::type_index>
        nameTypeIndexStoreInstance = {};
    return nameTypeIndexStoreInstance;
}


/// \brief Abstract component processor provides a type erased
/// interface for component creation and serialization
class TypeErasedProcessor
{
public:
    TypeErasedProcessor() = default;
    TypeErasedProcessor(const TypeErasedProcessor &) = default;
    TypeErasedProcessor(TypeErasedProcessor &&) = delete;
    TypeErasedProcessor &
    operator=(const TypeErasedProcessor &) = default;
    TypeErasedProcessor & operator=(TypeErasedProcessor &&) = delete;
    virtual ~TypeErasedProcessor() = default;
    virtual void addComponentToHandle(ad::ent::Handle<ad::ent::Entity> aHandle,
                           const JsonWitness && aData) = 0;
    virtual void serializeComponent(JsonWitness && aJson,
                           ad::ent::Handle<ad::ent::Entity> aHandle) = 0;
};

template <class T_type>
class TypedProcessor : public TypeErasedProcessor
{
public:
    TypedProcessor(const char * aName) :
        mName{aName}
    {}

    static std::shared_ptr<TypedProcessor<T_type>> bind(const char * aName);

    void addComponentToHandle(ad::ent::Handle<ad::ent::Entity> aHandle,
                   const JsonWitness && aData) override;

    void serializeComponent(JsonWitness && aData,
                   ad::ent::Handle<ad::ent::Entity> aHandle) override;

    const char * mName;
};

template <class T_type>
std::shared_ptr<TypeErasedProcessor> addConstructor(const char * aName)
{
    auto constructor = std::make_shared<TypedProcessor<T_type>>(aName);
    typedProcessorStore().insert_or_assign(std::type_index(typeid(T_type)), constructor);
    nameTypeIndexStore().insert_or_assign(aName, std::type_index(typeid(T_type)));
    return constructor;
}

template <class T_type>
std::shared_ptr<TypedProcessor<T_type>> TypedProcessor<T_type>::bind(const char * aName)
{
    return std::dynamic_pointer_cast<TypedProcessor<T_type>>(
        addConstructor<T_type>(aName));
}

#define REFLEXION_REGISTER(name)                                             \
    struct InitConstructor##name                                               \
    {                                                                          \
        static inline const std::shared_ptr<reflexion::TypedProcessor<name>> \
            a = reflexion::TypedProcessor<name>::bind(#name);                \
    };
} // namespace reflexion
