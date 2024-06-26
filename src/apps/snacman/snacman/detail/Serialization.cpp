#include "Serialization.h"

#include "handy/StringId.h"
#include "Reflexion.h"
#include "Reflexion_impl.h"

#include <nlohmann/json.hpp>
#include <string>

namespace ad {
namespace ent {

using json = nlohmann::json;
ent::Handle<ent::Entity> & operator&(ent::Handle<ent::Entity> & aHandle,
                                     json & aData)
{
    json components = aData["components"];

    for (const auto & comp : components)
    {
        std::string compName = comp["name"].get<std::string>();

        handy::StringId id{compName};
        snacgame::detail::HandleBinderStore::sStore.at(id)->construct(aHandle, comp);
    }

    return aHandle;
}

} // namespace ent
} // namespace ad
