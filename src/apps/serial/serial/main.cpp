#include "Serialization.h"
#include "comp.h"
#include "Reflexion.h"

#include <entity/EntityManager.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
int main (int argc, char *argv[]) {
    
    ad::ent::EntityManager world{};
    ad::ent::Handle<ad::ent::Entity> handle = world.addEntity();
    std::ifstream f("test.json");
    const json j = json::parse(f);
    // int a = 1;
    // auto b = serialNvp<int>{"yo", a};
    // j & b;
    // std::cout << a << std::endl;
    from_json(handle, j);
    std::cout << "hello" << handle.get()->get<component::CompA>().a << std::endl;
    json jOutput;
    to_json(handle, jOutput);
    std::cout << jOutput.dump(4) << std::endl;
    return 0;
}
