#pragma once


#include "Model.h"

#include <resource/ResourceFinder.h>

#include <filesystem>


namespace ad::renderer {


namespace semantic
{
    const Semantic gPosition{"Position"};
    const Semantic gNormal{"Normal"};
    const Semantic gUv{"Uv"};
    const Semantic gLocalToWorld{"LocalToWorld"};
    const Semantic gDiffuseTexture{"DiffuseTexture"};

    const BlockSemantic gFrame{"Frame"};
    const BlockSemantic gView{"View"};
} // namespace semantic


// IMPORTANT: for the moment, just load the first vertex stream in the file
// has to be extended to actually load complex models.
Node loadBinary(const std::filesystem::path & aBinaryFile, Storage & aStorage, Material aDefaultMaterial);


struct Loader
{
    /// @brief Load an effect file (.sefx), store it in `aStorage` and return its handle.
    Effect * loadEffect(const std::filesystem::path & aEffectFile,
                        Storage & aStorage,
                        const FeatureSet & aFeatures);

    /// @brief Load a `.prog` file as an IntrospectProgram.
    IntrospectProgram loadProgram(const filesystem::path & aProgFile);

    resource::ResourceFinder mFinder;
};

VertexStream makeFromPositions(Storage & aStorage, std::span<math::Position<3, GLfloat>> aPositions);

SemanticBufferViews makeInstanceStream(Storage & aStorage, std::size_t aInstanceCount);


} // namespace ad::renderer