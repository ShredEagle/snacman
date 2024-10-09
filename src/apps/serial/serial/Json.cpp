#include "Comp.h"
#include "reflexion/Reflexion.h"
#include "reflexion/Reflexion_impl.h"
#include "serialization/Witness.h"

#include "catch.hpp"
#include <chrono>
#include <cstring>
#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <math/Vector.h>
#include <nlohmann/json.hpp>
#include <reflexion/NameValuePair.h>

using nlohmann::json;
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
            witness_json(handle, Witness::make(&result));
            THEN("It contains the right data in the json")
            {
                CHECK(result.contains("components"));
                CHECK(result["components"]["CompA"]["aInt"] == 1);
                CHECK(result["components"]["CompA"]["aBool"] == true);
                CHECK(result["components"]["CompA"]["aString"] == "hello");
                REQUIRE_THAT(
                    14121.3f,
                    Catch::Matchers::WithinULP(
                        result["components"]["CompA"]["aFloat"].get<float>(),
                        0));
            }
        }
    }

    GIVEN("a handle with component containing vector, array, map... convert it "
          "to json")
    {
        ad::ent::EntityManager world{};
        ad::ent::Handle<ad::ent::Entity> handle = world.addEntity("handle");
        {
            ad::ent::Phase phase;
            handle.get(phase)->add(CompB{{0, 1, 2, 3, 4, 5, 6, 7},
                                         {3, 4, 5},
                                         {{"first", 1}, {"second", 2}},
                                         {{"third", 3}, {"fourth", 4}}});
        }

        WHEN("We serialize it")
        {
            json result;
            witness_json(handle, Witness::make(&result));
            THEN("It contains the right data in the json")
            {
                CHECK(result.contains("components"));
                json comp = result["components"]["CompB"];
                CHECK(comp["aArray"].size() == 8);
                for (size_t i = 0; i < comp["aArray"].size(); i++)
                {
                    CHECK(comp["aArray"].at(i) == (int) i);
                }
                CHECK(comp["aVector"].size() == 3);
                for (size_t i = 0; i < comp["aVector"].size(); i++)
                {
                    CHECK(comp["aVector"].at(i) == (int) (i + 3));
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

    GIVEN(
        "a handle with component containing another handle convert it to json")
    {
        ad::ent::EntityManager world{};
        ad::ent::Handle<ad::ent::Entity> handle = world.addEntity("handle");
        ad::ent::Handle<ad::ent::Entity> otherHandle = world.addEntity("other");
        {
            ad::ent::Phase phase;
            handle.get(phase)->add(CompC{otherHandle, {2}, {{0}, {1}, {2}}});
        }

        WHEN("We serialize it")
        {
            json result;
            witness_json(world, Witness::make(&result));
            THEN("it contains the right data in the json")
            {
                for (auto & [name, entity] : result.items())
                {
                    CHECK(entity.contains("components"));
                    if (name == "handle0")
                    {
                        CHECK(entity["components"]["CompC"].is_object());
                        CHECK(entity["components"]["CompC"]["aOther"]
                              == "other1");
                        CHECK(entity["components"]["CompC"]["aCompPart"]
                                  .is_object());
                        CHECK(entity["components"]["CompC"]["aCompPart"]["aInt"]
                              == 2);

                        auto & comp = entity["components"]["CompC"];
                        CHECK(comp["aCompPartVector"].is_array());
                        CHECK(comp["aCompPartVector"].size() == 3);
                        for (size_t i = 0; i < comp["aCompPartVector"].size();
                             i++)
                        {
                            CHECK(comp["aCompPartVector"].at(i)["aInt"]
                                  == (int) i);
                        }
                    }
                    else if (name == "other1")
                    {
                    }
                }
            }
        }
    }

    GIVEN("a handle with component containing snacman like data")
    {
        ad::ent::EntityManager world{};
        ad::ent::Handle<ad::ent::Entity> handle = world.addEntity("handle");
        std::chrono::high_resolution_clock::time_point nowTimePoint(
            std::chrono::high_resolution_clock::now());
        {
            ad::ent::Phase phase;
            CompSnacTest comp = CompSnacTest{{{0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}},
                         ControllerType::Gamepad, GameInput(69, {0.45f, 0.32f}, {0.12f, -0.23f}),
                         nowTimePoint};
            handle.get(phase)->add(comp);
        }
        WHEN("We serialize it")
        {
            json result;
            witness_json(world, Witness::make(&result));
            THEN("it contains the right data in the json") {}
            {
                for (auto & [name, entity] : result.items())
                {
                    CHECK(entity["components"]["CompSnacTest"].is_object());
                    CHECK(entity["components"]["CompSnacTest"]["mPlayerHitbox"]["mPosition"]["mStore"][0] == 0.f);
                    CHECK(entity["components"]["CompSnacTest"]["mPlayerHitbox"]["mPosition"]["mStore"][1] == 0.f);
                    CHECK(entity["components"]["CompSnacTest"]["mPlayerHitbox"]["mPosition"]["mStore"][2] == 0.f);
                    CHECK(entity["components"]["CompSnacTest"]["mPlayerHitbox"]["mDimension"]["mStore"][0] == 1.f);
                    CHECK(entity["components"]["CompSnacTest"]["mPlayerHitbox"]["mDimension"]["mStore"][1] == 1.f);
                    CHECK(entity["components"]["CompSnacTest"]["mPlayerHitbox"]["mDimension"]["mStore"][2] == 1.f);
                    CHECK(entity["components"]["CompSnacTest"]["mType"] == ControllerType::Gamepad);
                    CHECK(entity["components"]["CompSnacTest"]["mInput"]["mCommand"] == 69);
                    CHECK(entity["components"]["CompSnacTest"]["mInput"]["mLeftDirection"]["mStore"][0] == 0.45f);
                    CHECK(entity["components"]["CompSnacTest"]["mInput"]["mLeftDirection"]["mStore"][1] == 0.32f);
                    CHECK(entity["components"]["CompSnacTest"]["mInput"]["mRightDirection"]["mStore"][0] == 0.12f);
                    CHECK(entity["components"]["CompSnacTest"]["mInput"]["mRightDirection"]["mStore"][1] == -0.23f);
                    CHECK(entity["components"]["CompSnacTest"]["mStartTime"]["since_epoch"] == nowTimePoint.time_since_epoch().count());
                }
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
            testify_json(handle, Witness::make_const(&jInput));

            THEN("The handles contain the right components")
            {
                CHECK(handle.get()->has<CompA>());
                CHECK(handle.get()->get<CompA>().aInt == 1);
                CHECK(handle.get()->get<CompA>().aBool == false);
                CHECK(handle.get()->get<CompA>().aString == "AmAString");
                REQUIRE_THAT(1.034f, Catch::Matchers::WithinULP(
                                         handle.get()->get<CompA>().aFloat, 0));
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
                        "aArray": [0, 1, 2, 3, 4, 5, 6, 7],
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
            testify_json(handle, Witness::make_const(&jInput));

            THEN("The handles contain the right components")
            {
                CHECK(handle.get()->has<CompB>());
                auto comp = handle.get()->get<CompB>();
                CHECK(comp.aArray.size() == 8);
                for (size_t i = 0; i < comp.aArray.size(); i++)
                {
                    CHECK(comp.aArray.at(i) == (int) i);
                }
                CHECK(comp.aVector.size() == 3);
                for (size_t i = 0; i < comp.aVector.size(); i++)
                {
                    CHECK(comp.aVector.at(i) == (int) (i + 3));
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
            testify_json(world, Witness::make_const(&jInput));

            THEN("The handles contain the right components")
            {
                world.forEachHandle([](ad::ent::Handle<ad::ent::Entity> aHandle,
                                       const char * aName) {
                    if (strcmp(aName, "handle 0") == 0)
                    {
                        CHECK(aHandle.get()->has<CompC>());
                        auto comp = aHandle.get()->get<CompC>();
                        CHECK(comp.aCompPart.aInt == 420);
                        ad::ent::Handle<ad::ent::Entity> other = comp.aOther;
                        CHECK(strcmp(other.name(), "other 1") == 0);
                    }
                    else if (strcmp(aName, "other 1") == 0)
                    {
                    }
                    else
                    {
                        CHECK(false);
                    }
                });
            }
        }
    }
}
