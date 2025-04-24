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
    Tangent,
    Albedo,
    TextureCoords0,
    TextureCoords1,
    Joints0,
    Weights0,
    // Usually per instance
    LocalToWorld,
    InstancePosition,
    TextureOffset,
    MatrixPaletteOffset, // Usefull to index into a buffer of joint matrices for different instances
    BoundingBox,
    Bearing,
    GlyphIndex,
    // Usually uniform
    BaseColorFactor,
    BaseColorUVIndex,
    Gamma,
    NormalUVIndex, // Index of the normal texture coordinates in the array of UV coordinates
    NormalMapScale,
    AmbientColor,
    LightColor,
    LightPosition,
    LightViewingMatrix,
    WorldToCamera,
    Projection,
    ViewingMatrix, // Composed WorldToCamera and Projection matrices.
    FramebufferResolution,
    NearDistance,
    FarDistance,
    ShadowBias,
    RenormalizationRange,
    // Usually samplers
    BaseColorTexture,
    NormalTexture,
    MetallicRoughnessTexture,
    ShadowMap,
    FontAtlas, // TODO should it just be atlas, used for both sprites and fonts?
    //SpriteAtlas,

    //
    // Added while decommissioning away from renderer V1
    //
    EntityIdx, 
    MaterialIdx,

    _End/* Must be last
*/
};


// Note: Semantic for the whole block is likely to be application specific,
// we might need a mechanism for the application to provide this enumeration.

/// \brief Intended to apply high-level semantic to buffer-backed interface blocks.
enum class BlockSemantic
{
    Viewing, // camera pose and projection
    GlyphMetrics, // metrics for the glyphs found in the glyph atlas
    JointMatrices,

    //
    // Added while decommissioning away from renderer V1
    //
    Entities,
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