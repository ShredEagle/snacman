#pragma once


#include "Model.h"

#include <filesystem>


namespace ad::renderer {


namespace semantic
{
    const Semantic gPosition{"Position"};
    const Semantic gModelTransform{"ModelTransform"};

    const BlockSemantic gFrame{"Frame"};
    const BlockSemantic gView{"View"};
} // namespace semantic


// IMPORTANT: for the moment, just load the first vertex stream in the file
// has to be extended to actually load complex models.
VertexStream loadBinary(Storage & aStorage, const std::filesystem::path & aBinaryFile);

VertexStream makeFromPositions(Storage & aStorage, std::span<math::Position<3, GLfloat>> aPositions);

SemanticBufferViews makeInstanceStream(Storage & aStorage, std::size_t aInstanceCount);


} // namespace ad::renderer