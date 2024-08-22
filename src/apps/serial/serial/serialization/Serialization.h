#pragma once

#include "../SnacArchive.h"
#include <reflexion/Concept.h>
#include <reflexion/NameValuePair.h>
#include "../ReflexionFunctions.h"

#include <entity/Entity.h>
#include <math/Box.h>
#include <math/Vector.h>
#include <nlohmann/json.hpp>
#include <type_traits>
#include <utility>

using json = nlohmann::json;

inline std::vector<std::pair<ad::ent::Handle<ad::ent::Entity> *, std::string>> &
handleRequestsInstance()
{
    static std::vector<
        std::pair<ad::ent::Handle<ad::ent::Entity> *, std::string>>
        handleRequestsStore = {};
    return handleRequestsStore;
}
