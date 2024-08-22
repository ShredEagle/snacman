#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

// #include "comp.h"
// #include "entity/Entity.h"
// #include "reflexion/Reflexion.h"
// #include "reflexion/Reflexion_impl.h"
// #include "reflexion/Serialization.h"
//
// #include <entity/EntityManager.h>
// #include <fstream>
// #include <iostream>
// #include <nlohmann/json.hpp>
//
// using json = nlohmann::json;
//
// int main(int argc, char * argv[])
// {
//     ad::ent::EntityManager world{};
//     int i;
//     std::cout << "0: Deserialize, 1: Serialize, 2: both, 3: world deserialize, "
//                  "4: world serialize"
//               << std::endl;
//     std::cin >> i;
//     // int a = 1;
//     // auto b = serialNvp<int>{"yo", a};
//     // j & b;
//     // std::cout << a << std::endl;
//     ad::ent::Handle<ad::ent::Entity> handle = world.addEntity("handle");
//     ad::ent::Handle<ad::ent::Entity> other = world.addEntity("other");
//     json jOutput;
//     if (i == 0 || i == 2)
//     {
//         std::ifstream f("test_handle.json");
//         const json jInput = json::parse(f);
//         from_json(handle, jInput);
//         SnacArchiveOut archive(std::cout);
//         static_assert(can_be_described_to<SnacArchiveOut, component::CompA>);
//         if (handle.get()->has<component::CompA>())
//         {
//             archive & "CompA" & "\n"
//                 & handle.get()->get<component::CompA>() & "\n";
//         }
//     }
//     if (i == 1)
//     {
//         ad::ent::Phase phase;
//         handle.get(phase)->add(component::CompA{1, false, {"hello"}, other});
//     }
//     if (i == 1 || i == 2)
//     {
//         to_json(handle, jOutput);
//         std::cout << jOutput.dump(4) << std::endl;
//     }
//
//     if (i == 3)
//     {
//         std::ifstream f("test_world.json");
//         const json jInput = json::parse(f);
//         from_json(world, jInput);
//         SnacArchiveOut archive(std::cout);
//         world.forEachHandle([&archive](ad::ent::Handle<ad::ent::Entity> aHandle,
//                                        const char * aName) {
//             archive & "Entity : " & aName & "\n";
//             if (aHandle.get()->has<component::CompA>())
//             {
//                 archive & "CompA" & "\n"
//                     & aHandle.get()->get<component::CompA>() & "\n";
//             }
//         });
//     }
//
//     if (i == 4)
//     {
//         {
//             ad::ent::Phase phase;
//             handle.get(phase)->add(
//                 component::CompA{1, false, {"hello"}, other});
//         }
//         to_json(world, jOutput);
//         std::cout << jOutput.dump(4) << std::endl;
//     }
//     return 0;
// }
