#pragma once

#include <string>

namespace ad {
namespace snac {


enum class Semantic
{
    // TODO should we have separate vertex/instance semantics
    // Usally per vertex
    Position,
    Normal,
    Albedo,
    TextureCoords,
    // Usually per instance
    LocalToWorld,
    //Usually uniform
    AmbientColor,
    LightColor,
    LightPosition,
    WorldToCamera,
    Projection,

    _End/* Must be last
*/
};


std::string to_string(Semantic aSemantic);

Semantic to_semantic(std::string_view aAttributeName);

// Note: At the moment, the fact that an attribute is normalized or not
//       is hardcoded by its semantic. This is to be revisited when new need arises.
bool isNormalized(Semantic aSemantic);


} // namespace snac
} // namespace ad