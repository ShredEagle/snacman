#include "catch.hpp"
#include <cstring>

#include "entity/Entity.h"
#include "entity/EntityManager.h"

#include "serialization/Json.h"
#include "reflexion/Reflexion.h"
#include "reflexion/Reflexion_impl.h"

#include "serial/serialization/JsonWitness.h"
#include "serialization/Serialization.h"

#include <reflexion/NameValuePair.h>
#include <nlohmann/json.hpp>

using nlohmann::json;

struct CompA
{
    int aInt;
    bool aBool;
    std::string aString;
    float aFloat;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(aInt));
        aWitness.witness(NVP(aBool));
        aWitness.witness(NVP(aString));
        aWitness.witness(NVP(aFloat));
    }
};

REFLEXION_REGISTER(CompA)

struct CompB
{
    std::array<int, 3> aArray;
    std::vector<int> aVector;
    std::map<std::string, int> aMap;
    std::unordered_map<std::string, int> aUnorderedMap;
    ad::math::Vec<3, float> aMathVec;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(aArray));
        aWitness.witness(NVP(aVector));
        aWitness.witness(NVP(aMap));
        aWitness.witness(NVP(aUnorderedMap));
        aWitness.witness(NVP(aMathVec));
    }
};

REFLEXION_REGISTER(CompB)

struct CompPart
{
    int aInt;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(aInt));
    }
};

REFLEXION_REGISTER(CompPart)

struct CompC
{
    ad::ent::Handle<ad::ent::Entity> aOther;
    CompPart aCompPart;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(aOther));
        aWitness.witness(NVP(aCompPart));
    }
};

REFLEXION_REGISTER(CompC)

SCENARIO("Serialize a handle")
{
    GIVEN("a handle with component containing stl base type convert it to json")
    {
        ad::ent::EntityManager world{};
        ad::ent::Handle<ad::ent::Entity> handle = world.addEntity("handle");
        {
            ad::ent::Phase phase;
            handle.get(phase)->add(CompA{1, true, "hello", 14121.3f});
        }
        
        WHEN("We serialize it")
        {
            json result;
            to_json(handle, JsonWitness::make(result));
            THEN("It contains the right data in the json")
            {
                CHECK(result.contains("components"));
                CHECK(result["components"]["CompA"]["aInt"] == 1);
                CHECK(result["components"]["CompA"]["aBool"] == true);
                CHECK(result["components"]["CompA"]["aString"] == "hello");
                REQUIRE_THAT(14121.3f, Catch::Matchers::WithinULP(result["components"]["CompA"]["aFloat"].get<float>(), 0));
            }
        }
    }

    GIVEN("a handle with component containing vector, array, map... convert it to json")
    {
        ad::ent::EntityManager world{};
        ad::ent::Handle<ad::ent::Entity> handle = world.addEntity("handle");
        {
            ad::ent::Phase phase;
            handle.get(phase)->add(CompB{{0,1,2}, {3,4,5}, {{"first", 1}, {"second", 2}}, {{"third", 3}, {"fourth", 4}}});
        }
        
        WHEN("We serialize it")
        {
            json result;
            to_json(handle, JsonWitness::make(result));
            THEN("It contains the right data in the json")
            {
                CHECK(result.contains("components"));
                json comp = result["components"]["CompB"];
                CHECK(comp["aArray"].size() == 3);
                for (int i = 0; i < comp["aArray"].size(); i++)
                {
                    CHECK(comp["aArray"].at(i) == i);
                }
                CHECK(comp["aVector"].size() == 3);
                for (int i = 0; i < comp["aVector"].size(); i++)
                {
                    CHECK(comp["aVector"].at(i) == i + 3);
                }
                CHECK(comp["aMap"].size() == 2);
                CHECK(comp["aMap"].at("first") == 1);
                CHECK(comp["aMap"].at("second") == 2);
                CHECK(comp["aUnorderedMap"].size() == 2);
                CHECK(comp["aUnorderedMap"].at("third") == 3);
                CHECK(comp["aUnorderedMap"].at("fourth") == 4);
            }
        }
    }
}

SCENARIO("Deserialize json into a handle or a world")
{
    GIVEN("a world, a handle and a raw json string containing base stl type")
    {
        ad::ent::EntityManager world{};
        ad::ent::Handle<ad::ent::Entity> handle = world.addEntity("handle");
        nlohmann::json jInput = nlohmann::json::parse(R"(
            {
                "components": {
                    "CompA": {
                        "aInt": 1,
                        "aBool": false,
                        "aString": "AmAString",
                        "aFloat": 1.034
                    }
                }
            }
        )");

        WHEN("json is deserialized")
        {
            from_json(handle, JsonWitness::make_const(jInput));

            THEN("The handles contain the right components")
            {
                CHECK(handle.get()->has<CompA>());
                CHECK(handle.get()->get<CompA>().aInt == 1);
                CHECK(handle.get()->get<CompA>().aBool == false);
                CHECK(handle.get()->get<CompA>().aString == "AmAString");
                REQUIRE_THAT(1.034f, Catch::Matchers::WithinULP(handle.get()->get<CompA>().aFloat, 0));
            }
        }
    }

    GIVEN("a world, a handle and a raw json string containing vector and array")
    {
        ad::ent::EntityManager world{};
        ad::ent::Handle<ad::ent::Entity> handle = world.addEntity("handle");
        nlohmann::json jInput = nlohmann::json::parse(R"(
            {
                "components": {
                    "CompB": {
                        "aArray": [0, 1, 2],
                        "aVector": [3, 4, 5],
                        "aMap": {
                            "first": 1,
                            "second": 2
                        },
                        "aUnorderedMap": {
                            "third": 3,
                            "fourth": 4
                        },
                        "aMathVec": {
                            "mStore": [1.0, 1.1, 1.2]
                        }
                    }
                }
            }
        )");

        WHEN("json is deserialized")
        {
            from_json(handle, JsonWitness::make_const(jInput));

            THEN("The handles contain the right components")
            {
                CHECK(handle.get()->has<CompB>());
                auto comp = handle.get()->get<CompB>();
                CHECK(comp.aArray.size() == 3);
                for (int i = 0; i < comp.aArray.size(); i++)
                {
                    CHECK(comp.aArray.at(i) == i);
                }
                CHECK(comp.aVector.size() == 3);
                for (int i = 0; i < comp.aVector.size(); i++)
                {
                    CHECK(comp.aVector.at(i) == i + 3);
                }
                CHECK(comp.aMap.size() == 2);
                CHECK(comp.aMap.at("first") == 1);
                CHECK(comp.aMap.at("second") == 2);
                CHECK(comp.aUnorderedMap.size() == 2);
                CHECK(comp.aUnorderedMap.at("third") == 3);
                CHECK(comp.aUnorderedMap.at("fourth") == 4);
            }
        }
    }

    GIVEN("a world and a raw json string")
    {
        ad::ent::EntityManager world{};
        nlohmann::json jInput = nlohmann::json::parse(R"(
            {
                "handle": {
                    "components": {
                        "CompC": {
                            "aOther": "other",
                            "aCompPart": {
                                "aInt": 420
                            }
                        }
                    }
                },
                "other": {
                    "components": null
                }
            }
        )");

        WHEN("json is deserialized")
        {
            from_json(world, JsonWitness::make_const(jInput));

            THEN("The handles contain the right components")
            {
                world.forEachHandle([](ad::ent::Handle<ad::ent::Entity> aHandle, const char * aName) {
                    if (strcmp(aName, "handle 0") == 0)
                    {
                        CHECK(aHandle.get()->has<CompC>());
                        auto comp = aHandle.get()->get<CompC>();
                        CHECK(comp.aCompPart.aInt == 420);
                        ad::ent::Handle<ad::ent::Entity> other = comp.aOther;
                        CHECK(strcmp(other.name(), "other 1") == 0);
                    }
                    else if (strcmp(aName, "other 1") == 0)
                    {}
                    else
                    {
                        CHECK(false);
                    }
                });
            }
        }
    }
}
