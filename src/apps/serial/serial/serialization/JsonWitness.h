#pragma once

#include "Json.h"
#include "serial/serialization/Serialization.h"

#include <nlohmann/json.hpp>
#include <reflexion/NameValuePair.h>

using nlohmann::json;

class JsonWitness
{
private:
    JsonWitness(json & aData) :
        mData(aData)
    {};
    JsonWitness(json && aData) :
        mData(aData)
    {};
public:
    static const JsonWitness make_const(json & aData)
    {
        return JsonWitness(aData);
    }
    static JsonWitness make(json & aData)
    {
        return JsonWitness(aData);
    }

    ~JsonWitness() = default;
    JsonWitness(JsonWitness &) = delete;
    JsonWitness(JsonWitness &&) = delete;
    JsonWitness &operator=(JsonWitness &&) & = delete;
    JsonWitness &operator=(const JsonWitness &) & = delete;
    JsonWitness &operator=(JsonWitness &&) && = delete;
    JsonWitness &operator=(const JsonWitness &) && = delete;

    json & mData;

    template <class T_data>
    void witness(ad::reflex::Nvp<T_data> d)
    {
        auto & [name, value] = d;
        witness(name, value);
    }

    template <class T_data>
    void witness(ad::reflex::Nvp<T_data> d) const
    {
        auto & [name, value] = d;
        witness(name, value);
    }

    template <class T_value>
    void witness(const char * aName, T_value * aValue)
    {
        static_assert(false, "Default witness template should not be called");
    }

    template <class T_value>
    void witness(const char * aName, T_value * aValue) const
    {
        static_assert(false,
                      "Default const witness template should not be called");
    }

    template <JsonSerializable T_value>
        requires(
            !ad::reflex::can_be_described_to<T_value, JsonWitness>
            && !ad::reflex::can_be_described_to_by_fn<T_value, JsonWitness>)
    void witness(const char * aName, T_value * aValue)
    {
        mData[aName] = *aValue;
    }

    template <class T_value>
        requires(
            !JsonSerializable<T_value>
            || ad::reflex::can_be_described_to<T_value, JsonWitness>
            || ad::reflex::can_be_described_to_by_fn<T_value, JsonWitness>)
    void witness(const char * aName, T_value * aValue)
    {
        JsonWitness temp{mData[aName]};
        aValue->describeTo(temp);
    }

    template <>
    void witness(const char * aName, ad::ent::Handle<ad::ent::Entity> * aHandle)
    {
        mData[aName] = aHandle->name();
    }

    template <JsonExtractable T_value>
        requires(
            !ad::reflex::can_be_described_to<T_value, JsonWitness>
            && !ad::reflex::can_be_described_to_by_fn<T_value, JsonWitness>)
    void witness(const char * aName, T_value * aValue) const
    {
        *aValue = mData[aName].template get<T_value>();
    }

    template <class T_value>
        requires(
            !JsonExtractable<T_value>
            || ad::reflex::can_be_described_to<T_value, JsonWitness>
            || ad::reflex::can_be_described_to_by_fn<T_value, JsonWitness>)
    void witness(const char * aName, T_value * aValue) const
    {
        const JsonWitness temp{mData[aName]};
        aValue->describeTo(temp);
    }

    template <>
    void witness(const char * aName,
                 ad::ent::Handle<ad::ent::Entity> * aHandle) const
    {
        handleRequestsInstance().emplace_back(aHandle, mData[aName]);
    }
};
