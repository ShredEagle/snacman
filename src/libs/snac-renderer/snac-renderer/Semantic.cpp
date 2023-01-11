#include "Semantic.h"

#include <handy/StringUtilities.h>

#include <stdexcept>

#include <cassert>


namespace ad {
namespace snac {


std::string to_string(Semantic aSemantic)
{
#define MAPPING(semantic) case Semantic::semantic: return #semantic;
    switch(aSemantic)
    {
        MAPPING(Position)
        MAPPING(Normal)
        MAPPING(Albedo)
        MAPPING(TextureCoords)
        MAPPING(LocalToWorld)
        MAPPING(AmbientColor) 
        MAPPING(LightColor)
        MAPPING(LightPosition)
        MAPPING(WorldToCamera)
        MAPPING(Projection)
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
#undef MAPPING
}

Semantic to_semantic(std::string_view aResourceName)
{
    static const std::string delimiter = "_";
    std::string_view prefix, body, suffix;
    std::tie(prefix, body) = lsplit(aResourceName, delimiter);
    std::tie(body, suffix) = lsplit(body, delimiter);

#define MAPPING(name) else if(body == #name) { return Semantic::name; }
    if(false){}
    MAPPING(Position)
    MAPPING(Normal)
    MAPPING(Albedo)
    MAPPING(TextureCoords)
    MAPPING(LocalToWorld)
    MAPPING(AmbientColor) 
    MAPPING(LightColor)
    MAPPING(LightPosition)
    MAPPING(WorldToCamera)
    MAPPING(Projection)
    else
    {
        throw std::logic_error{
            "Unable to map shader resource name '" + std::string{aResourceName} + "' to a semantic."};
    }
#undef MAPPING
}


bool isNormalized(Semantic aSemantic)
{
    switch(aSemantic)
    {
        case Semantic::Albedo:
        case Semantic::AmbientColor:
        case Semantic::LightColor:
        {
            return true;
        }
        case Semantic::Position:
        case Semantic::Normal:
        case Semantic::TextureCoords:
        case Semantic::LocalToWorld:
        case Semantic::LightPosition:
        case Semantic::WorldToCamera:
        case Semantic::Projection:
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


std::string to_string(BlockSemantic aBlockSemantic)
{
#define MAPPING(semantic) case BlockSemantic::semantic: return #semantic;
    switch(aBlockSemantic)
    {
        MAPPING(Viewing)
        default:
        {
            auto value = static_cast<std::underlying_type_t<Semantic>>(aBlockSemantic);
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
#undef MAPPING
}


BlockSemantic to_blockSemantic(std::string_view aBlockName)
{
    std::string_view body = aBlockName;
#define MAPPING(name) else if(body == #name"Block") { return BlockSemantic::name; }
    if(false){}
    MAPPING(Viewing)
    else
    {
        if (aBlockName.ends_with(']'))
        {
            // TODO How to support array of blocks.
            throw std::logic_error{
                "Unable to map shader block name '" + std::string{aBlockName} + "' to a semantic, arrays of blocks are not supported."};
        }
        else
        {
            throw std::logic_error{
                "Unable to map shader block name '" + std::string{aBlockName} + "' to a semantic."};
        }
    }
#undef MAPPING
}


} // namespace snac
} // namespace ad