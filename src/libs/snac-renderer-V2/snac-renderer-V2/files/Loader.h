#pragma once


#include "../Model.h"

#include <math/Homogeneous.h>

#include <resource/ResourceFinder.h>

#include <filesystem>


namespace ad::renderer {


struct InstanceData
{
    GLuint mModelTransformIdx = (GLuint)-1;
    GLuint mMaterialIdx = (GLuint)-1;
};


// TODO move to a more generic header
// TODO review the usefulness of aVertexStream last argument
// alongside makeVertexStream
struct AttributeDescription
{
    Semantic mSemantic;
    // To replace wih graphics::ClientAttribute if we support interleaved buffers (i.e. with offset)
    graphics::AttributeDimension mDimension;
    GLenum mComponentType;   // data individual components' type.
};


// TODO move to a more generic header, review the usefulness of aVertexStream last argument
/// @note Provide a distinct buffer for each attribute stream at the moment (i.e. no interleaving).
/// @param aVertexStream If not nullptr, create views into this vertex stream buffers instead of creating
/// new buffers.
Handle<VertexStream> makeVertexStream(unsigned int aVerticesCount,
                                      unsigned int aIndicesCount,
                                      std::span<const AttributeDescription> aBufferedStreams,
                                      Storage & aStorage,
                                      const GenericStream & aStream,
                                      // This is probably only useful to create debug data buffers
                                      // (as there is little sense to not reuse the existing views)
                                      const VertexStream * aVertexStream = nullptr);


// IMPORTANT: for the moment, just load the first vertex stream in the file
// has to be extended to actually load complex models.
Node loadBinary(const std::filesystem::path & aBinaryFile,
                Storage & aStorage,
                Effect * aPartsEffect,
                // TODO should be replaced by the Handle to a shared VertexStream (when it is properly reused),
                // stream already containing the instance data
                const GenericStream & aStream/*to provide instance data, such as model transform*/);


/// @brief Load a triangle and a cube models in the same buffer objects (they do not share vertices though).
std::pair<Node, Node> loadTriangleAndCube(Storage & aStorage, 
                                          Effect * aPartsEffect,
                                          const GenericStream & aStream);


// TODO Ad 2024/02/15: Those free functins should be relocated
// (not even sure we want the library to propose an instance stream format at all)
GenericStream makeInstanceStream(Storage & aStorage, std::size_t aInstanceCount);

/// @brief Create a GL buffer of specified size (without loading data into it).
/// @return Buffer view to the buffer.
BufferView createBuffer(GLsizei aElementSize,
                        GLsizeiptr aElementCount,
                        GLuint aInstanceDivisor,
                        GLenum aHint,
                        Storage & aStorage);


Handle<ConfiguredProgram> storeConfiguredProgram(IntrospectProgram aProgram, Storage & aStorage);


struct Loader
{
    /// @brief Load an effect file (.sefx), store it in `aStorage` and return its handle.
    Effect * loadEffect(const std::filesystem::path & aEffectFile,
                        Storage & aStorage,
                        // TODO Ad 2023/10/03: #shader-system Remove this temporary.
                        const std::vector<std::string> & aDefines_temp = {}) const;

    /// @brief Load a `.prog` file as an IntrospectProgram.
    IntrospectProgram loadProgram(const filesystem::path & aProgFile,
                                  // TODO Ad 2023/10/03: #shader-system Remove this temporary.
                                  std::vector<std::string> aDefines_temp = {}) const;

    graphics::ShaderSource loadShader(const filesystem::path & aShaderFile) const;

    resource::ResourceFinder mFinder;
};


} // namespace ad::renderer