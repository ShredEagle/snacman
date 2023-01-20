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
    // Usually uniform
    InstancePosition,
    AmbientColor,
    LightColor,
    LightPosition,
    WorldToCamera,
    Projection,
    TextureOffset,
    BoundingBox,
    FramebufferResolution,
    // Usually samplers
    FontAtlas, // TODO should it just be atlas, used for both sprites and fonts?
    //SpriteAtlas,

    _End/* Must be last
*/
};


// Note: Semantic for the whole block is likely to be application specific,
// we might need a mechanism for the application to provide this enumeration.

/// \brief Intended to apply high-level semantic to buffer-backed interface blocks.
enum class BlockSemantic
{
    Viewing, // camera pose and projection
};


std::string to_string(Semantic aSemantic);

Semantic to_semantic(std::string_view aResourceName);

// Note: At the moment, the fact that an attribute is normalized or not
//       is hardcoded by its semantic. This is to be revisited when new need arises.
bool isNormalized(Semantic aSemantic);

std::string to_string(BlockSemantic aBlockSemantic);

BlockSemantic to_blockSemantic(std::string_view aBlockName);

} // namespace snac
} // namespace ad