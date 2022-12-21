#include "Semantic.h"

#include <handy/StringUtilities.h>

#include <stdexcept>

#include <cassert>


namespace ad {
namespace snac {


std::string to_string(Semantic aSemantic)
{
#define MAPPING_STR(semantic) case Semantic::semantic: return #semantic;
    switch(aSemantic)
    {
        MAPPING_STR(Position)
        MAPPING_STR(Normal)
        MAPPING_STR(Albedo)
        MAPPING_STR(TextureCoords)
        MAPPING_STR(LocalToWorld)
        default:
        {
            auto value = static_cast<std::underlying_type_t<Semantic>>(aSemantic);
            if(value >= static_cast<std::underlying_type_t<Semantic>>(Semantic::_End))
            {
                throw std::invalid_argument{"There are no semantics with value: " + std::to_string(value) + "."};
            }
            else
            {
                // Stop us here in a debug build
                assert(false);
                return "(Semantic#" + std::to_string(value) + ")";
            }
        }
    }
#undef MAPPING_STR
}

Semantic to_semantic(std::string_view aAttributeName)
{
    static const std::string delimiter = "_";
    std::string_view prefix, body, suffix;
    std::tie(prefix, body) = lsplit(aAttributeName, delimiter);
    std::tie(body, suffix) = lsplit(body, delimiter);

#define MAPPING(name) else if(body == #name) { return Semantic::name; }
    if(false){}
    MAPPING(Position)
    MAPPING(Normal)
    MAPPING(Albedo)
    MAPPING(TextureCoords)
    MAPPING(LocalToWorld)
    else
    {
        throw std::logic_error{
            "Unable to map shader attribute name '" + std::string{aAttributeName} + "' to a semantic."};
    }
#undef MAPPING
}


bool isNormalized(Semantic aSemantic)
{
    switch(aSemantic)
    {
        case Semantic::Albedo:
        {
            return true;
        }
        case Semantic::Position:
        case Semantic::Normal:
        case Semantic::TextureCoords:
        case Semantic::LocalToWorld:
        {
            return false;
        }
        default:
        {
            throw std::logic_error{
                "Semantic '" + to_string(aSemantic) + "' normalization has not been listed."};
        }
    }
}


} // namespace snac
} // namespace ad