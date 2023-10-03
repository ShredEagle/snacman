#pragma once


#include "Model.h"

#include <math/Homogeneous.h>

#include <resource/ResourceFinder.h>

#include <filesystem>


namespace ad::renderer {


struct InstanceData
{
    GLuint mModelTransformIdx = (GLuint)-1;
    GLuint mMaterialIdx = (GLuint)-1;
};


namespace semantic
{
    const Semantic gPosition{"Position"};
    const Semantic gNormal{"Normal"};
    const Semantic gColor{"Color"};
    const Semantic gUv{"Uv"};
    const Semantic gDiffuseTexture{"DiffuseTexture"};
    const Semantic gModelTransformIdx{"ModelTransformIdx"};
    const Semantic gMaterialIdx{"MaterialIdx"};

    const BlockSemantic gFrame{"Frame"};
    const BlockSemantic gView{"View"};
    const BlockSemantic gMaterials{"Materials"};
    const BlockSemantic gLocalToWorld{"LocalToWorld"};
} // namespace semantic


// IMPORTANT: for the moment, just load the first vertex stream in the file
// has to be extended to actually load complex models.
Node loadBinary(const std::filesystem::path & aBinaryFile,
                Storage & aStorage,
                Material aDefaultMaterial,
                // TODO should be replaced by the Handle to a shared VertexStream (when it is properly reused),
                // stream already containing the instance data
                const GenericStream & aStream/*to provide instance data, such as model transform*/);


struct Loader
{
    /// @brief Load an effect file (.sefx), store it in `aStorage` and return its handle.
    Effect * loadEffect(const std::filesystem::path & aEffectFile,
                        Storage & aStorage,
                        // TODO Ad 2023/10/03: #shader-system Remove this temporary.
                        const std::vector<std::string> & aDefines_temp/*,
                        const FeatureSet & aFeatures*/);

    /// @brief Load a `.prog` file as an IntrospectProgram.
    IntrospectProgram loadProgram(const filesystem::path & aProgFile,
                                    // TODO Ad 2023/10/03: #shader-system Remove this temporary.
                                  const std::vector<std::string> & aDefines_temp);

    resource::ResourceFinder mFinder;
};

VertexStream makeFromPositions(Storage & aStorage, std::span<math::Position<3, GLfloat>> aPositions);

GenericStream makeInstanceStream(Storage & aStorage, std::size_t aInstanceCount);


} // namespace ad::renderer