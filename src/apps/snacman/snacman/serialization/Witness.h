#pragma once

#ifndef GUARD_SERIAL_IMPORT
//Include Serial.h instead of Witness.h;
#endif

#include "Imgui.h"
#include "ImguiSerialization.h"
#include "imguiui/ImguiUi.h"
#include "Json.h"
#include "JsonSerialization.h"
#include "snacman/Resources.h"

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <reflexion/Concept.h>
#include <reflexion/NameValuePair.h>
#include <snac-reflexion/Reflexion.h>
#include <utility>
#include <variant>
#include <ranges>

namespace ad {
namespace snacgame {
struct GameContext;
}
namespace serial {

using imguiui::ImguiUi;
using nlohmann::json;

/// WITNESS_TYPE must be a list of template
/// parameter, all type must be pointer type (or equivalent)
/// To add a new type you must add it to WITNESS_TYPE
/// A new branch must be added to the two base witness methods
/// of the witness class with the new type and how to process data
/// for it
#define WITNESS_TYPE json *, ImguiUi *

typedef std::variant<WITNESS_TYPE> WitnessData;

class Witness;

template <class T_renderable>
concept ImguiRenderable =
    ImguiDefaultRenderable<T_renderable>;

template <class T_witnessable>
concept ImguiWitnessable =
    !ImguiDefaultRenderable<T_witnessable>
    && (reflex::can_be_described_to<T_witnessable, Witness>
        || reflex::can_be_described_to_by_fn<T_witnessable, Witness>);

template <class T_type>
concept JsonExtractable =
    JsonDefaultExtractable<T_type>
    && !reflex::can_be_described_to<T_type, Witness>
    && !reflex::can_be_described_to_by_fn<T_type, Witness>;
template <class T_type>
concept JsonTestifyable = !JsonDefaultExtractable<T_type>
                          || reflex::can_be_described_to<T_type, Witness>
                          || reflex::can_be_described_to_by_fn<T_type, Witness>;

template <class T_type>
concept JsonSerializable =
    JsonDefaultSerializable<T_type>
    && !reflex::can_be_described_to<T_type, Witness>
    && !reflex::can_be_described_to_by_fn<T_type, Witness>;
template <class T_type>
concept JsonWitnessable = !JsonDefaultSerializable<T_type>
                          || reflex::can_be_described_to<T_type, Witness>
                          || reflex::can_be_described_to_by_fn<T_type, Witness>;

namespace {
template <class T>
struct is_variant
{
    static constexpr bool value = false;
};

template <class... Ts>
struct is_variant<std::variant<Ts...>>
{
    static constexpr bool value = true;
};

template <class T>
constexpr bool is_variant_v = is_variant<T>::value;

template <std::size_t N>
struct variant_switch
{
    template <class T_variant, class T_witness>
    void operator()(std::size_t aIndex,
                    const T_witness & aWitness,
                    T_variant & aVariant) const
    {
        if (aIndex == N)
        {
            aWitness.witness("value", &std::get<N>(aVariant));
        }
        else
        {
            variant_switch<N - 1>{}(aIndex, aWitness, aVariant);
        }
    }
};
template <>
struct variant_switch<0>
{
    template <class T_variant, class T_witness>
    void operator()(std::size_t aIndex,
                    const T_witness & aWitness,
                    T_variant & aVariant) const
    {
        if (aIndex == 0)
        {
            aWitness.witness("value", &std::get<0>(aVariant));
        }
        else
        {
#ifndef NDEBUG
            throw std::runtime_error(
                "Error while deserializing json to variant: invalid index");
#endif
        }
    }
};
} // namespace

/// \brief Contains information about a json file and the entities created
/// by that json file
struct EntityLedger
{
    filesystem::path mSourcePath;
    std::vector<ent::Handle<ent::Entity>> mEntities;
    json mJson;
    ent::EntityManager & mWorld;
};

/// \brief Wrapper class for serialization data porcelain
/// based around a variant that is made of all available data porcelain type
class Witness
{
private:
    Witness(WitnessData aData, snacgame::GameContext & aContext) :
        mData(aData), mGameContext(aContext)
    {}

public:
    WitnessData mData;
    snacgame::GameContext & mGameContext;

    static const Witness make_const(WitnessData aData,
                                    snacgame::GameContext & aContext)
    {
        return Witness(aData, aContext);
    }
    static Witness make(WitnessData aData, snacgame::GameContext & aContext)
    {
        return Witness(aData, aContext);
    }

    /// Unfortunately it is not possible to forbid Witness Instantiation
    /// The first reason is that it is not possible to forbid assignement of
    /// prvalues (which the result of make and make_const is)
    /// It is also impossible to assert if a value is a prvalue or not
    /// These constructor declaration are here to prevent all other way to
    /// manipulate non temporary instance of Witness
    ~Witness() = default;
    Witness(const Witness &) = delete;
    Witness(Witness &&) = delete;
    Witness & operator=(Witness &&) & = delete;
    Witness & operator=(const Witness &) & = delete;
    Witness & operator=(Witness &&) && = delete;
    Witness & operator=(const Witness &&) && = delete;

    template <class T_data>
    void witness(reflex::Nvp<T_data> && d)
    {
        auto & [name, value] = d;
        witness(name, value);
    }

    template <class T_data>
    void witness(reflex::Nvp<T_data> && d) const
    {
        auto & [name, value] = d;
        witness(name, value);
    }

    template <class T_value>
    void witness(const char * aName, T_value * aValue)
    {
        std::visit(
            [this, aValue, aName](auto && arg) {
                using witness_type = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<witness_type, json *>)
                {
                    witness_json(aName, aValue);
                }
                else if constexpr (std::is_same_v<witness_type, ImguiUi *>)
                {
                    witness_imgui(aName, aValue);
                }
                else
                {
                    //static_assert(
                    //    false, "Default witness template should not be called");
                }
            },
            mData);
    }

    template <class T_value>
    void witness(const char * aName, T_value * aValue) const
    {
        std::visit(
            [this, aValue, aName](auto && arg) {
                using witness_type = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<witness_type, json *>)
                {
                    testify_json(aName, aValue);
                }
                else if constexpr (std::is_same_v<witness_type, ImguiUi *>)
                {
                }
                else
                {
                    //static_assert(
                    //    !std::is_same_v<T_value, T_value>, /*always false, but politely using template parameter*/
                    //    "Default witness template should not be called");
                }
            },
            mData);
    }

    template<class T_value>
    void describeWithMemberOrFunction(const Witness && aWitness, T_value & aValue) const
    {
        if constexpr(reflex::can_be_described_to_by_fn<T_value, Witness>)
        {
            describeTo(aWitness, aValue);
        }
        else
        {
            aValue.describeTo(aWitness);
        }
    }

    template<class T_value>
    void describeWithMemberOrFunction(const Witness & aWitness, T_value & aValue) const
    {
        if constexpr(reflex::can_be_described_to_by_fn<T_value, Witness>)
        {
            describeTo(aWitness, aValue);
        }
        else
        {
            aValue.describeTo(aWitness);
        }
    }

    template<class T_value>
    void describeWithMemberOrFunction(Witness && aWitness, T_value & aValue)
    {
        if constexpr(reflex::can_be_described_to_by_fn<T_value, Witness>)
        {
            describeTo(aWitness, aValue);
        }
        else
        {
            aValue.describeTo(aWitness);
        }
    }

    template<class T_value>
    void describeWithMemberOrFunction(Witness & aWitness, T_value & aValue)
    {
        if constexpr(reflex::can_be_described_to_by_fn<T_value, Witness>)
        {
            describeTo(aWitness, aValue);
        }
        else
        {
            aValue.describeTo(aWitness);
        }
    }

    template <ImguiRenderable T_value>
    void witness_imgui(const char * aName, T_value * aValue)
    {
        debugRender(aName, *aValue);
    }

    template <ImguiWitnessable T_value,
              class... T_args,
              template <class...>
              class T_range>
        requires(std::ranges::range<T_range<T_value, T_args...>>
                 && !std::is_same_v<T_range<T_value, T_args...>, std::string>)
    void witness_imgui(const char * aName, T_range<T_value, T_args...> * aRange)
    {
        static int clickedNode = -1;
        if (ImGui::TreeNode(aName))
        {
            float child_w = ImGui::GetContentRegionAvail().x;
            ImGui::BeginChild("rangechild", ImVec2(child_w, 200.0f));
            for (std::size_t i = 0; i < aRange->size(); i++)
            {
                ImGui::Text("%ld", i);

                if (ImGui::IsItemClicked())
                {
                    clickedNode = (int) i;
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            if (clickedNode > -1)
            {
                describeWithMemberOrFunction(*this, aRange->at(clickedNode));
            }
            ImGui::TreePop();
        }
        else
        {
            clickedNode = 0;
        }
    }

    template <ImguiWitnessable... T_value_types,
              template <class...>
              class T_variant>
        requires(is_variant_v<T_variant<T_value_types...>>)
    void witness_imgui(const char * aName,
                      T_variant<T_value_types...> * aVariant)
    {
        std::visit(
            [&](auto && value) {
                describeWithMemberOrFunction(*this, value);
            },
            *aVariant);
    }


    template <ImguiWitnessable T_value>
    void witness_imgui(const char * aName, T_value * aValue)
    {
        if (ImGui::TreeNode(aName))
        {
            describeWithMemberOrFunction(*this, *aValue);
            ImGui::TreePop();
        }
    }

    template <class T_value>
    void witness_imgui(const char * aName, T_value * aValue)
    {
        ImGui::Text("%s: No debug render for this type", aName);
    }

    // This is to witness reference to handle in components, not to be confused
    // with the free function witness_imgui that witness Handle as a proper
    // value This should put the name of the handle into the field
    void witness_imgui(const char * aName, ent::Handle<ent::Entity> * aHandle)
    {
        if (ImGui::TreeNode(aName))
        {
            ImGui::Text("%s", aHandle->name());
            ImGui::TreePop();
        }
    }

    void witness_imgui(const char * aName,
                      renderer::Handle<const renderer::Object> * aObject);

    void witness_imgui(const char * aName,
                      std::shared_ptr<snac::Resources::LoadedFont_t> * aObject);

    template <JsonSerializable T_value>
    void witness_json(const char * aName, T_value * aValue)
    {
        json & data = *std::get<json *>(mData);
        data[aName] = *aValue;
    }

    template <JsonWitnessable T_value,
              typename... T_args,
              template <class...>
              class T_range>
        requires(std::ranges::range<T_range<T_value, T_args...>>)
    void witness_json(const char * aName, T_range<T_value, T_args...> * aRange)
    {
        json & data = *std::get<json *>(mData);
        data[aName] = json::array();
        int i = 0;
        for (T_value & value : *aRange)
        {
            describeWithMemberOrFunction(Witness::make(&data[aName][i++], mGameContext), value);
        }
    }

    template <JsonWitnessable... T_value_types,
              template <class...>
              class T_variant>
        requires(is_variant_v<T_variant<T_value_types...>>)
    void witness_json(const char * aName,
                      T_variant<T_value_types...> * aVariant)
    {
        json & data = *std::get<json *>(mData);
        data[aName]["variant_index"] = aVariant->index();
        std::visit(
            [&](auto && value) {
                describeWithMemberOrFunction(Witness::make(&data[aName]["value"], mGameContext), value);
            },
            *aVariant);
    }

    template <JsonWitnessable T_value>
    void witness_json(const char * aName, T_value * aValue)
    {
        json & data = *std::get<json *>(mData);
        describeWithMemberOrFunction(Witness::make(&data[aName], mGameContext), *aValue);
    }

    // This is to witness a reference to handle in components, not to be
    // confused with the free function witness_json that witness Handle as a
    // proper value This should put the name of the handle into the json data
    void witness_json(const char * aName, ent::Handle<ent::Entity> * aHandle)
    {
        json & data = *std::get<json *>(mData);
        if (aHandle->isValid())
        {
            data[aName] = aHandle->name();
        }
    }

    void witness_json(const char * aName,
                      renderer::Handle<const renderer::Object> * aObject);

    void witness_json(const char * aName,
                      std::shared_ptr<snac::Resources::LoadedFont_t> * aObject);

    template <JsonExtractable T_value>
    void testify_json(const char * aName, T_value * aValue) const
    {
        json & data = *std::get<json *>(mData);
        *aValue = data[aName].template get<T_value>();
    }

    template <JsonTestifyable T_value>
    void testify_json(const char * aName, T_value * aValue) const
    {
        json & data = *std::get<json *>(mData);
        describeWithMemberOrFunction(Witness::make_const(&data[aName], mGameContext), *aValue);
    }

    template <JsonTestifyable T_value,
              class... T_args,
              template <class...>
              class T_range>
        requires(std::ranges::range<T_range<T_value, T_args...>>)
    void testify_json(const char * aName,
                      T_range<T_value, T_args...> * aRange) const
    {
        json & data = *std::get<json *>(mData);
        for (auto & value : data[aName])
        {
            aRange->emplace_back();
            describeWithMemberOrFunction(Witness::make_const(&value, mGameContext), aRange->back());
        }
    }

    template <JsonWitnessable... T_value_types,
              template <class...>
              class T_variant>
        requires(is_variant_v<T_variant<T_value_types...>>)
    void testify_json(const char * aName,
                      T_variant<T_value_types...> * aVariant) const
    {
        json & data = *std::get<json *>(mData);
        const std::size_t index = aVariant->index();
        variant_switch<sizeof...(T_value_types) - 1>{}(
            index, Witness::make_const(&data[aName], mGameContext), *aVariant);
    }

    // This is to testify a reference to handle in components, not to be
    // confused with the free function witness_json that witness Handle as a
    // proper value This should make a request to associate the handle pointer
    // to the proper handle once all the entities have been created
    void testify_json(const char * aName,
                      ent::Handle<ent::Entity> * aHandle) const
    {
        json & data = *std::get<json *>(mData);
        // TODO(franz): should add the invalid handle to nameHandleMap
        if (data.contains(aName))
        {
            reflexion::handleRequestsInstance().emplace_back(aHandle,
                                                             data[aName]);
        }
    }

    void testify_json(const char * aName,
                      renderer::Handle<const renderer::Object> * aObject) const;

    void testify_json(const char * aName,
                      std::shared_ptr<snac::Resources::LoadedFont_t> * aObject) const;
};

void resolveRequestsInstance(snacgame::GameContext & aWorld);

#define WITNESS_FUNC_DECLARATION(name)                                         \
    std::vector<ent::Handle<ent::Entity>> testify_##name(ent::EntityManager & aWorld, const Witness && aData);  \
    void testify_##name(ent::Handle<ent::Entity> & aHandle,                    \
                        const Witness && aData);                               \
    void witness_##name(                      \
        ent::EntityManager & aWorld, Witness && aData);                        \
    void witness_##name(                      \
        ent::Handle<ent::Entity> & aHandle, Witness && aData);

WITNESS_FUNC_DECLARATION(json)
WITNESS_FUNC_DECLARATION(imgui)
EntityLedger loadLedgerFromJson(const filesystem::path & aJsonPath,
                          ent::EntityManager & aWorld,
                          snacgame::GameContext & aContext);
} // namespace serial
} // namespace ad

#undef GUARD_SERIAL_IMPORT
