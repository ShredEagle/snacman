#include "Loader.h"

#include "BinaryArchive.h" 
#include "Cube.h"
#include "GlApi.h"
#include "Json.h"
#include "Logging.h"
#include "RendererReimplement.h"

#include <arte/Image.h>
// TODO Ad 2023/10/06: Move the Json <-> Math type converters to a better place
#include <arte/detail/GltfJson.h>

#include <math/Transformations.h>

#include <renderer/BufferLoad.h>
#include <renderer/ShaderSource.h>
#include <renderer/Texture.h>
#include <renderer/utilities/FileLookup.h>


namespace ad::renderer {


namespace {

    // TODO move to math library
    template <std::floating_point T_value>
    bool isWithinTolerance(T_value aLhs, T_value aRhs, T_value aRelativeTolerance)
    {
        T_value diff = std::abs(aLhs - aRhs);
        T_value maxMagnitude = std::max(std::abs(aLhs), std::abs(aRhs));
        return diff <= (maxMagnitude * aRelativeTolerance);
    }

    // TODO move to a more generic library (graphics?)
    Pose decompose(const math::AffineMatrix<4, GLfloat> & aTransformation)
    {
        Pose result {
            .mPosition = aTransformation.getAffine(),
        };

        std::array<math::Vec<3, GLfloat>, 3> rows{
            math::Vec<3, GLfloat>{aTransformation[0]},
            math::Vec<3, GLfloat>{aTransformation[1]},
            math::Vec<3, GLfloat>{aTransformation[2]},
        };
        math::Size<3, GLfloat> scale{
            rows[0].getNorm(),
            rows[1].getNorm(),
            rows[2].getNorm(),
        };

        // TODO we will need to introduce some tolerance here
        assert(isWithinTolerance(scale[0], 1.f, 1E-6f));
        assert(isWithinTolerance(scale[1], 1.f, 1E-6f));
        assert(isWithinTolerance(scale[2], 1.f, 1E-6f));
        result.mUniformScale = 1;
        //assert(scale[0] == scale[1] && scale[0] == scale[2]);
        //result.mUniformScale = scale[0];

        return result;
    }


    /// @brief Create a GL buffer of specified size (without loading data into it).
    /// @return Buffer view to the buffer.
    BufferView createBuffer(GLsizei aElementSize,
                            GLsizeiptr aElementCount,
                            GLuint aInstanceDivisor,
                            GLenum aHint,
                            Storage & aStorage)
    {
        graphics::BufferAny glBuffer; // glGenBuffers()
        // Note: Using a random target, the underlying buffer objects are all identical.
        constexpr auto target = graphics::BufferType::Array;
        graphics::ScopedBind boundBuffer{glBuffer, target}; // glBind()

        const GLsizeiptr bufferSize = aElementSize * aElementCount;

        gl.BufferData(
            static_cast<GLenum>(target),
            bufferSize,
            nullptr,
            aHint);

        aStorage.mBuffers.push_back(std::move(glBuffer));
        graphics::BufferAny * buffer = &aStorage.mBuffers.back();

        return BufferView{
            .mGLBuffer = buffer,
            .mStride = aElementSize,
            .mInstanceDivisor = aInstanceDivisor,
            .mOffset = 0,
            .mSize = bufferSize, // The view has access to the whole buffer ATM
        };
    };

    BufferView makeBufferView(graphics::BufferAny * aBuffer,
                              GLsizei aElementSize,
                              GLsizeiptr aElementCount,
                              GLuint aInstanceDivisor,
                              GLintptr aOffsetIntoBuffer)
    {
        const GLsizeiptr bufferSize = aElementSize * aElementCount;

        return BufferView{
            .mGLBuffer = aBuffer,
            .mStride = aElementSize,
            .mInstanceDivisor = aInstanceDivisor,
            .mOffset = aOffsetIntoBuffer,
            .mSize = bufferSize, // The view has access to the provided range of elements
        };
    };

    /// @brief Load data into an existing buffer, with offset aElementFirst.
    template <class T_element>
    void loadBuffer(const graphics::BufferAny & aBuffer, 
                    GLuint aElementFirst,
                    std::span<T_element> aCpuBuffer)
    {
        const std::size_t elementSize = sizeof(T_element);

        // Note: Using a random target, the underlying buffer objects are all identical.
        constexpr auto target = graphics::BufferType::Array;
        graphics::ScopedBind boundBuffer{aBuffer, target}; // glBind()

        gl.BufferSubData(
            static_cast<GLenum>(target),
            aElementFirst * elementSize, // offset
            aCpuBuffer.size_bytes(), // data length
            aCpuBuffer.data());
    };


    // TODO Ad 2023/10/11: #loader Get rid of those hardcoded types when the binary streams are dynamic.
    constexpr auto gPositionSize = 3 * sizeof(float); // TODO that is crazy coupling
    constexpr auto gNormalSize = gPositionSize;
    constexpr auto gUvSize = 2 * sizeof(float);
    constexpr auto gColorSize = 4 * sizeof(float);
    constexpr auto gIndexSize = sizeof(unsigned int);

    struct AttributeDescription
    {
        Semantic mSemantic;
        // To replace wih graphics::ClientAttribute if we support interleaved buffers (i.e. with offset)
        graphics::AttributeDimension mDimension;
        GLenum mComponentType;   // data individual components' type.
    };

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
                                          const VertexStream * aVertexStream = nullptr)
    {
        // TODO Ad 2023/10/11: Should we support smaller index types.
        using Index_t = GLuint;
        BufferView iboView = [&]()
        {
            if(aVertexStream)
            {
                const BufferView & previousIndexBufferView = aVertexStream->mIndexBufferView;
                GLintptr indexOffset = previousIndexBufferView.mSize;
                graphics::BufferAny * existingBuffer = aVertexStream->mIndexBufferView.mGLBuffer;
                return makeBufferView(existingBuffer, sizeof(Index_t), aIndicesCount, 0, indexOffset);
            }
            else
            {
                return createBuffer(sizeof(Index_t), aIndicesCount, 0, GL_STATIC_DRAW, aStorage);
            }
        }();

        // The consolidated vertex stream
        aStorage.mVertexStreams.push_back({
            .mVertexBufferViews{aStream.mVertexBufferViews},
            .mSemanticToAttribute{aStream.mSemanticToAttribute},
            .mIndexBufferView = iboView,
            .mIndicesType = graphics::MappedGL_v<Index_t>,
        });

        VertexStream & vertexStream = aStorage.mVertexStreams.back();

        for(const auto & attribute : aBufferedStreams)
        {
            const GLsizei attributeSize = 
                attribute.mDimension.countComponents() * graphics::getByteSize(attribute.mComponentType);

            BufferView attributeView = [&]()
            {
                if(aVertexStream)
                {
                    // Will throw if there is no buffer for the attribute's semantic in the source VertexStream.
                    const BufferView & previousBufferView = 
                        aVertexStream->mVertexBufferViews.at(
                            aVertexStream->mSemanticToAttribute.at(attribute.mSemantic).mBufferViewIndex);
                    graphics::BufferAny * attributeBuffer = previousBufferView.mGLBuffer;

                    return makeBufferView(attributeBuffer, attributeSize, aVerticesCount, 0, previousBufferView.mSize);
                }
                else
                {
                    return createBuffer(attributeSize, aVerticesCount, 0, GL_STATIC_DRAW, aStorage);
                }
            }();

            vertexStream.mSemanticToAttribute.emplace(
                attribute.mSemantic,
                AttributeAccessor{
                    .mBufferViewIndex = vertexStream.mVertexBufferViews.size(), // view is added next
                    .mClientDataFormat{
                        .mDimension = attribute.mDimension,
                        .mOffset = 0,
                        .mComponentType = attribute.mComponentType,
                    },
                }
            );

            vertexStream.mVertexBufferViews.push_back(attributeView);
        }

        return &vertexStream;
    }


    // TODO Do we want attribute interleaving, or are we content with one distinct buffer per attribute ?
    // (the attribute interleaving should then be done on the exporter side).
    Part loadMesh(BinaryInArchive & aIn, 
                  const VertexStream & aVertexStream,
                  // TODO #inout replace this inout by something more sane
                  GLuint & aVertexFirst,
                  GLuint & aIndexFirst,
                  Material aPartsMaterial)
    {
        // Load data into an existing buffer, with offset aElementFirst.
        auto loadBufferFromArchive = 
            [&aIn](graphics::BufferAny & aBuffer, 
                   GLuint aElementFirst,
                   GLsizei aElementSize,
                   std::size_t aElementCount)
            {
                std::size_t bufferSize = aElementCount * aElementSize;
                std::unique_ptr<char[]> cpuBuffer = aIn.readBytes(bufferSize);
                loadBuffer(aBuffer, aElementFirst * aElementSize, std::span{cpuBuffer.get(), bufferSize});
            };

        // TODO #loader Those hardcoded indices are smelly, hard-coupled to the attribute streams structure in the binary.
        graphics::BufferAny & positionBuffer = *(aVertexStream.mVertexBufferViews.end() - 4)->mGLBuffer;
        graphics::BufferAny & normalBuffer = *(aVertexStream.mVertexBufferViews.end() - 3)->mGLBuffer;
        graphics::BufferAny & colorBuffer = *(aVertexStream.mVertexBufferViews.end() - 2)->mGLBuffer;
        graphics::BufferAny & uvBuffer = *(aVertexStream.mVertexBufferViews.end() - 1)->mGLBuffer;
        graphics::BufferAny & indexBuffer = *(aVertexStream.mIndexBufferView.mGLBuffer);

        const std::string meshName = aIn.readString();

        unsigned int materialIndex;
        aIn.read(materialIndex);

        unsigned int verticesCount;
        aIn.read(verticesCount);
        
        // load positions
        loadBufferFromArchive(positionBuffer, aVertexFirst, gPositionSize, verticesCount);
        // load normals
        loadBufferFromArchive(normalBuffer, aVertexFirst, gNormalSize, verticesCount);

        unsigned int colorChannelsCount;
        aIn.read(colorChannelsCount);
        assert(colorChannelsCount <= 1);
        for(unsigned int colorIdx = 0; colorIdx != colorChannelsCount; ++colorIdx)
        {
            loadBufferFromArchive(colorBuffer, aVertexFirst, gColorSize, verticesCount);
        }

        unsigned int uvChannelsCount;
        aIn.read(uvChannelsCount);
        assert(uvChannelsCount <= 1);
        
        // TODO handle no UV channel by adding all of them at the end? (so it is not problem we index out of range)
        assert(uvChannelsCount == 1);

        for(unsigned int uvIdx = 0; uvIdx != uvChannelsCount; ++uvIdx)
        {
            loadBufferFromArchive(uvBuffer, aVertexFirst, gUvSize, verticesCount);
        }

        unsigned int primitiveCount;
        aIn.read(primitiveCount);
        const unsigned int indicesCount = 3 * primitiveCount;
        loadBufferFromArchive(indexBuffer, aIndexFirst, gIndexSize, indicesCount);

        // Assign the part material index to the otherwise common material
        aPartsMaterial.mPhongMaterialIdx = materialIndex;
        Part part{
            .mName = std::move(meshName),
            .mMaterial = aPartsMaterial,
            .mVertexStream = &aVertexStream,
            .mPrimitiveMode = GL_TRIANGLES,
            .mVertexFirst = aVertexFirst,
            .mVertexCount = verticesCount,
            .mIndexFirst = aIndexFirst,
            .mIndicesCount = indicesCount,
        };

        aIn.read(part.mAabb);

        aVertexFirst += verticesCount;
        aIndexFirst += indicesCount;
        return part;
    }


    Object * addObject(Storage & aStorage)
    {
        aStorage.mObjects.emplace_back();
        return &aStorage.mObjects.back();
    }


    Node loadNode(BinaryInArchive & aIn,
                  Storage & aStorage,
                  const Material aDefaultMaterial,
                  const VertexStream & aVertexStream,
                  // TODO #inout replace this inout by something more sane
                  GLuint & aVertexFirst,
                  GLuint & aIndexFirst)
    {
        std::string name = aIn.readString();

        unsigned int meshesCount, childrenCount;
        aIn.read(meshesCount);
        aIn.read(childrenCount);

        math::Matrix<4, 3, GLfloat> localTransformation;
        aIn.read(localTransformation);

        Node node{
            .mInstance{
                .mObject = (meshesCount > 0) ? addObject(aStorage) : nullptr,
                .mPose = decompose(math::AffineMatrix<4, GLfloat>{localTransformation}),
                .mName = std::move(name),
            },
        };

        for(std::size_t meshIdx = 0; meshIdx != meshesCount; ++meshIdx)
        {
            node.mInstance.mObject->mParts.push_back(
                loadMesh(aIn, aVertexStream, aVertexFirst, aIndexFirst, aDefaultMaterial)
            );
        }
        
        math::Box<float> objectAabb;
        aIn.read(objectAabb);
        if(node.mInstance.mObject != nullptr)
        {
            node.mInstance.mObject->mAabb = objectAabb;
        }

        for(std::size_t childIdx = 0; childIdx != childrenCount; ++childIdx)
        {
            node.mChildren.push_back(loadNode(aIn, aStorage, aDefaultMaterial, aVertexStream, aVertexFirst, aIndexFirst));
        }

        aIn.read(node.mAabb);

        return node;
    }


    void loadTextureLayer(graphics::Texture & a3dTexture,
                          GLint aLayerIdx,
                          std::filesystem::path aTexturePath,
                          math::Size<2, int> aExpectedDimensions)
    {
        arte::Image<math::sdr::Rgba> image{aTexturePath, arte::ImageOrientation::InvertVerticalAxis};
        assert(aExpectedDimensions == image.dimensions());

        proto::writeTo(a3dTexture,
                       static_cast<const std::byte *>(image),
                       graphics::InputImageParameters::From(image),
                       math::Position<3, GLint>{0, 0, aLayerIdx});
    }


    // TODO Ad 2023/08/01: 
    // Review how the effects (the programs) are provided to the parts (currently hardcoded)
    void loadMaterials(BinaryInArchive & aIn,
                       graphics::UniformBufferObject & aDestinationUbo,
                       graphics::Texture & aDestinationTexture,
                       std::vector<Name> & aDestinationNames)
    {
        unsigned int materialsCount;
        aIn.read(materialsCount);

        {
            std::vector<PhongMaterial> materials{materialsCount};
            aIn.read(std::span{materials});

            graphics::load(aDestinationUbo, std::span{materials}, graphics::BufferHint::StaticDraw);
        }

        { // load material names
            unsigned int namesCount; // TODO: useless, but this is because the filewritter wrote the number of elements in a vector
            aIn.read(namesCount);
            assert(namesCount == materialsCount);

            aDestinationNames.reserve(aDestinationNames.size() + materialsCount);
            for(unsigned int nameIdx = 0; nameIdx != materialsCount; ++nameIdx)
            {
                aDestinationNames.push_back(aIn.readString());
            }
        }

        math::Size<2, int> imageSize;        
        aIn.read(imageSize);

        unsigned int pathsCount;
        aIn.read(pathsCount);

        if(pathsCount > 0)
        {
            assert(imageSize.width() > 0 && imageSize.height() > 1);

            graphics::Texture textureArray{GL_TEXTURE_2D_ARRAY};
            graphics::ScopedBind boundTextureArray{textureArray};
            gl.TexStorage3D(textureArray.mTarget, 
                            graphics::countCompleteMipmaps(imageSize),
                            graphics::MappedSizedPixel_v<math::sdr::Rgba>,
                            imageSize.width(), imageSize.height(), pathsCount);
            { // scoping `isSuccess`
                GLint isSuccess;
                glGetTexParameteriv(textureArray.mTarget, GL_TEXTURE_IMMUTABLE_FORMAT, &isSuccess);
                if(!isSuccess)
                {
                    SELOG(error)("Cannot create immutable storage for textures array.");
                    throw std::runtime_error{"Error creating immutable storage for textures."};
                }
            }

            for(unsigned int pathIdx = 0; pathIdx != pathsCount; ++pathIdx)
            {
                std::string path = aIn.readString();
                loadTextureLayer(textureArray, pathIdx, aIn.mParentPath / path, imageSize);
            }

            glGenerateMipmap(textureArray.mTarget);

            aDestinationTexture = std::move(textureArray);
        }
        else
        {
            aDestinationTexture = graphics::Texture{GL_TEXTURE_2D_ARRAY, graphics::Texture::NullTag{}};
        }
    }


} // unnamed namespace


std::pair<Node, Node> loadTriangleAndCube(Storage & aStorage, 
                                          Effect * aPartsEffect,
                                          const GenericStream & aStream)
{
    // Hardcoded attributes descriptions
    static const std::array<AttributeDescription, 3> gAttributeStreamsInBinary {
        AttributeDescription{
            .mSemantic = semantic::gPosition,
            .mDimension = 3,
            .mComponentType = GL_FLOAT,
        },
        AttributeDescription{
            .mSemantic = semantic::gNormal,
            .mDimension = 3,
            .mComponentType = GL_FLOAT,
        },
        AttributeDescription{
            .mSemantic = semantic::gColor,
            .mDimension = 4,
            .mComponentType = GL_FLOAT,
        },
    };

    std::vector<math::Position<3, GLfloat>> trianglePositions;
    std::vector<math::Vec<3, GLfloat>> triangleNormals;
    std::vector<math::hdr::Rgba_f> triangleColors{
        math::hdr::gRed<GLfloat>,
        math::hdr::gGreen<GLfloat>,
        math::hdr::gBlue<GLfloat>,
    };
    for(unsigned int vertex = 0; vertex != triangle::Maker::gVertexCount; ++vertex)
    {
        // Translated outside a side of the cube, to make it obvious if the cube is not offseting 
        // its view into the shared vertex buffers.
        trianglePositions.push_back(triangle::Maker::getPosition(vertex) + math::Vec<3, GLfloat>{0.f, 0.f, -1.1f});
        triangleNormals.push_back(triangle::Maker::getNormal(vertex));
    }
    std::vector<GLuint> triangleIndices{0, 1, 2};

    auto cubePositions = getExpandedCubePositions();
    auto cubeNormals = getExpandedCubeNormals();
    std::vector<math::hdr::Rgba_f> cubeColors(cubePositions.size(), math::hdr::gWhite<GLfloat>);
    assert(cubePositions.size() == cubeNormals.size());

    //TODO handle non-indexed geometry in draw(), so we can get rid of the sequential indices.
    std::vector<GLuint> cubeIndices(cubePositions.size());
    std::iota(cubeIndices.begin(), cubeIndices.end(), 0);

    const unsigned int verticesCount = (unsigned int)(trianglePositions.size() + cubePositions.size());
    const unsigned int indicesCount = (unsigned int)(triangleIndices.size() + cubeIndices.size());

    // Prepare the common buffer storage for the whole binary (one buffer / attribute)
    Handle<VertexStream> triangleStream = makeVertexStream(
        verticesCount,
        indicesCount,
        gAttributeStreamsInBinary,
        aStorage,
        aStream);
    
    
    // This is the pendant of hardcoded vertex attributes description, and must be kept in sync
    // (which is a code smell, we should do better)
    // The buffer views from aStream have been added at the beginning of the vertex buffer views
    // so we fetch back from the end.
    BufferView & positionBufferView = *(triangleStream->mVertexBufferViews.end() - 3);
    BufferView & normalBufferView   = *(triangleStream->mVertexBufferViews.end() - 2);
    BufferView & colorBufferView    = *(triangleStream->mVertexBufferViews.end() - 1);
    BufferView & indexBufferView    = triangleStream->mIndexBufferView;

    // We want each shape to use distinct BufferViews (so distinct VertexStream)
    // to test BufferView offsets.
    // Manually get there.
    positionBufferView.mSize = sizeof(GLfloat) * 3 * trianglePositions.size();
    normalBufferView.mSize   = sizeof(GLfloat) * 3 * trianglePositions.size();
    colorBufferView.mSize    = sizeof(GLfloat) * 4 * trianglePositions.size();
    indexBufferView.mSize    = sizeof(GLuint) * triangleIndices.size();

    Handle<VertexStream> cubeStream = makeVertexStream(
        (GLuint)cubePositions.size(),
        (GLuint)cubeIndices.size(),
        gAttributeStreamsInBinary,
        aStorage,
        aStream,
        triangleStream);

    // Load the buffers of the vertex streams

    auto load =
        [](Handle<VertexStream>  aStream,
           GLuint aFirstIndex,  // write offset
           auto aIndices, auto aPositions, auto aNormals, auto aColors)
        {
            const graphics::BufferAny & indexBuffer = *aStream->mIndexBufferView.mGLBuffer;

            loadBuffer(indexBuffer, aFirstIndex, std::span{aIndices});

            const graphics::BufferAny & positionBuffer = *(aStream->mVertexBufferViews.end() - 3)->mGLBuffer;
            const graphics::BufferAny & normalBuffer   = *(aStream->mVertexBufferViews.end() - 2)->mGLBuffer;
            const graphics::BufferAny & colorBuffer    = *(aStream->mVertexBufferViews.end() - 1)->mGLBuffer;

            loadBuffer(positionBuffer, aFirstIndex, std::span{aPositions});
            loadBuffer(normalBuffer,   aFirstIndex, std::span{aNormals});
            loadBuffer(colorBuffer,    aFirstIndex, std::span{aColors});
        };

    GLuint cubeFirstVertex = (GLuint)trianglePositions.size();
    load(triangleStream, 0, triangleIndices, trianglePositions, triangleNormals, triangleColors);
    load(cubeStream, cubeFirstVertex, cubeIndices, cubePositions, cubeNormals, cubeColors);

    //
    // Push the objects
    //

    // Triangle
    math::Vec<3, GLfloat> positionOffset = {0.f, 0.f, 1.5f};

    math::Box<GLfloat> aabb{
        .mPosition = {-1.f, -1.f, 0.f},
        .mDimension = {2.f, 2.f, 0.f},
    };
    aStorage.mObjects.push_back(Object{
        .mParts = {
            Part{
                .mMaterial = Material{
                    .mNameArrayOffset = 1, // With the -1 (no-material) material index, this will index the first element in the common material names array
                    .mEffect = aPartsEffect,
                },
                .mVertexStream = triangleStream,
                .mPrimitiveMode = GL_TRIANGLES,
                .mVertexFirst = 0,
                .mVertexCount = (GLuint)trianglePositions.size(),
                // TODO Ad 2023/10/12: Remove the need for indices
                .mIndexFirst = 0,
                .mIndicesCount = (GLuint)triangleIndices.size(),
                .mAabb = aabb,
            }
        },
        .mAabb = aabb,
    });

    Node triangleNode{
        .mInstance = Instance{
            .mObject = &aStorage.mObjects.back(),
            .mPose = Pose{
                .mPosition = -positionOffset,
            },
            .mName = "triangle",
        },
        .mAabb = aabb * math::trans3d::translate(-positionOffset),
    };

    // Cube
    aabb.mDimension.depth() = 2;
    aStorage.mObjects.push_back(Object{
        .mParts = {
            Part{
                .mMaterial = Material{
                    .mNameArrayOffset = 1,
                    .mEffect = aPartsEffect,
                },
                .mVertexStream = cubeStream,
                .mPrimitiveMode = GL_TRIANGLES,
                .mVertexFirst = 0,
                .mVertexCount = (GLuint)cubePositions.size(),
                // TODO Ad 2023/10/12: Remove the need for indices
                .mIndexFirst = 0,
                .mIndicesCount = (GLuint)cubeIndices.size(),
                .mAabb = aabb,
            }
        },
        .mAabb = aabb,
    });

    Node cubeNode{
        .mInstance = Instance{
            .mObject = &aStorage.mObjects.back(),
            .mPose = Pose{
                .mPosition = positionOffset,
            },
            .mName = "cube",
        },
        .mAabb = aabb * math::trans3d::translate(positionOffset),
    };

    // Return the nodes, one per shape
    return {triangleNode, cubeNode};
}


// TODO should not take a default material, but have a way to get the actual effect to use 
// (maybe directly from the binary file)
Node loadBinary(const std::filesystem::path & aBinaryFile,
                Storage & aStorage,
                Effect * aPartsEffect,
                const GenericStream & aStream)
{
    BinaryInArchive in{
        .mIn{std::ifstream{aBinaryFile, std::ios::binary}},
        .mParentPath{aBinaryFile.parent_path()},
    };

    // TODO we need a unified design where the load code does not duplicate the type of the write.
    unsigned int version;
    in.read(version);
    assert(version == 1);

    unsigned int verticesCount = 0;
    unsigned int indicesCount = 0;
    in.read(verticesCount);
    in.read(indicesCount);


    // Binary attributes descriptions
    // TODO Ad 2023/10/11: #loader Support dynamic set of attributes in the binary
    static const std::array<AttributeDescription, 4> gAttributeStreamsInBinary {
        AttributeDescription{
            .mSemantic = semantic::gPosition,
            .mDimension = 3,
            .mComponentType = GL_FLOAT,
        },
        AttributeDescription{
            .mSemantic = semantic::gNormal,
            .mDimension = 3,
            .mComponentType = GL_FLOAT,
        },
        AttributeDescription{
            .mSemantic = semantic::gColor,
            .mDimension = 4,
            .mComponentType = GL_FLOAT,
        },
        AttributeDescription{
            .mSemantic = semantic::gUv,
            .mDimension = 2,
            .mComponentType = GL_FLOAT,
        },
    };

    // Prepare the single buffer storage for the whole binary
    VertexStream * consolidatedStream = makeVertexStream(
        verticesCount,
        indicesCount,
        gAttributeStreamsInBinary,
        aStorage,
        aStream);

    // Prepare a MaterialContext for the whole binary, 
    // I.e. in a scene, each entry (each binary) gets its own material UBO and its own Texture array.
    // TODO #azdo #perf we could load textures and materials in a single buffer for a whole scene to further consolidate
    // TODO #assetprocessor If materials were loaded first, then loadMaterials could directly create the UBO and Texture
    //    * no need to add a texture if the model does not use them.
    {
        aStorage.mUbos.emplace_back();
        aStorage.mTextures.emplace_back(GL_TEXTURE_2D_ARRAY); // Will be overwritten anyway

        aStorage.mMaterialContexts.push_back(MaterialContext{
            .mUboRepo = {{semantic::gMaterials, &aStorage.mUbos.back()}},
            .mTextureRepo = {{semantic::gDiffuseTexture, &aStorage.mTextures.back()}},
        });
    }
    Material partsMaterial{
        // The material names is a single array for all binaries
        // the binary-local material index needs to be offset to index into the common name array.
        .mNameArrayOffset = aStorage.mMaterialNames.size(),
        .mContext = &aStorage.mMaterialContexts.back(),
        .mEffect = aPartsEffect,
    };

    GLuint vertexFirst = 0;
    GLuint indexFirst = 0;
    Node result = loadNode(in, aStorage, partsMaterial, *consolidatedStream, vertexFirst, indexFirst);

    loadMaterials(in, aStorage.mUbos.back(), aStorage.mTextures.back(), aStorage.mMaterialNames);

    return result;
}


Effect * Loader::loadEffect(const std::filesystem::path & aEffectFile,
                            Storage & aStorage,
                            const std::vector<std::string> & aDefines_temp)
{
    aStorage.mEffects.emplace_back();
    Effect & result = aStorage.mEffects.back();
    result.mName = aEffectFile.string();

    Json effect = Json::parse(std::ifstream{mFinder.pathFor(aEffectFile)});
    for (const auto & technique : effect.at("techniques"))
    {
        aStorage.mPrograms.push_back(ConfiguredProgram{
            .mProgram = loadProgram(technique.at("programfile"), aDefines_temp),
            // TODO Share Configs between programs that are compatibles (i.e., same input semantic and type at same attribute index)
            // (For simplicity, we create one Config per program at the moment).
            .mConfig = (aStorage.mProgramConfigs.emplace_back(), &aStorage.mProgramConfigs.back()),
        });
        result.mTechniques.push_back(Technique{
            .mConfiguredProgram = &aStorage.mPrograms.back(),
        });

        if(technique.contains("annotations"))
        {
            Technique & inserted = result.mTechniques.back();
            for(auto [category, value] : technique.at("annotations").items())
            {
                inserted.mAnnotations.emplace(category, value.get<std::string>());
            }
        }
    }
    return &result;
}


IntrospectProgram Loader::loadProgram(const filesystem::path & aProgFile,
                                      const std::vector<std::string> & aDefines_temp)
{
    std::vector<std::pair<const GLenum, graphics::ShaderSource>> shaders;

    auto programPath = mFinder.pathFor(aProgFile);

    // TODO factorize and make more robust (e.g. test file existence)
    Json program;
    try 
    {
        program = Json::parse(std::ifstream{programPath});
    }
    catch (Json::parse_error &)
    {
        SELOG(critical)("Rethrowing exception from attempt to parse Json from '{}'.", aProgFile.string());
        throw;
    }

    // TODO decide if we want the shader path to be relative to the prog file (with FileLookup)
    // or to be found in the current "assets" folders of ResourceFinder.
    graphics::FileLookup lookup{programPath};

    for (auto [shaderStage, shaderFile] : program.items())
    {
        GLenum stageEnumerator;
        if(shaderStage == "features")
        {
            // Special case, not a shader stage
            continue;
        }
        else if(shaderStage == "vertex")
        {
            stageEnumerator = GL_VERTEX_SHADER;
        }
        else if (shaderStage == "fragment")
        {
            stageEnumerator = GL_FRAGMENT_SHADER;
        }
        else
        {
            SELOG(critical)("Unable to map shader stage key '{}' to a program stage.", shaderStage);
            throw std::invalid_argument{"Unhandled shader stage key."};
        }
        
        auto [inputStream, identifier] = lookup(shaderFile);
        shaders.emplace_back(
            stageEnumerator,
            graphics::ShaderSource::Preprocess(*inputStream, aDefines_temp, identifier, lookup));
    }

    SELOG(debug)("Compiling shader program from '{}', containing {} stages.", lookup.top(), shaders.size());

    return IntrospectProgram{shaders.begin(), shaders.end(), aProgFile.filename().string()};
}


Scene Loader::loadScene(const filesystem::path & aSceneFile,
                        const GenericStream & aStream,
                        Storage & aStorage)
{
    Scene scene;

    filesystem::path scenePath = mFinder.pathFor(aSceneFile);
    Json description = Json::parse(std::ifstream{mFinder.pathFor(aSceneFile)});
    filesystem::path workingDir = scenePath.parent_path();

    for (const auto & entry : description.at("entries"))
    {
        std::vector<std::string> defines = entry["features"].get<std::vector<std::string>>();
        filesystem::path modelFile = entry.at("model");

        Node model = loadBinary(workingDir / modelFile,
                                aStorage,
                                loadEffect(entry["effect"], aStorage, defines),
                                aStream);

        // TODO store and load names in the binary file format
        SELOG(info)("Loaded model '{}' with bouding box {}.",
                     modelFile.stem().string(), fmt::streamed(model.mAabb));

        math::Box<GLfloat> poserAabb = model.mAabb;

        Pose pose;
        if(entry.contains("pose"))
        {
            pose.mPosition = entry.at("pose").value<math::Vec<3, float>>("position", {});
            pose.mUniformScale = entry.at("pose").value("uniform_scale", 1.f);

            poserAabb *= math::trans3d::scaleUniform(pose.mUniformScale)
                        * math::trans3d::translate(pose.mPosition);
        }

        Node modelPoser{
            .mInstance = Instance{
                .mPose = pose,
                .mName = entry.at("name").get<std::string>(),
            },
            .mChildren = {std::move(model)},
            .mAabb = poserAabb,
        };
        scene.addToRoot(std::move(modelPoser));
    }

    return scene;
}


GenericStream makeInstanceStream(Storage & aStorage, std::size_t aInstanceCount)
{
    BufferView vboView = createBuffer(sizeof(InstanceData),
                                      aInstanceCount,
                                      1,
                                      GL_STREAM_DRAW,
                                      aStorage);

    return GenericStream{
        .mVertexBufferViews = { vboView, },
        .mSemanticToAttribute{
            {
                semantic::gModelTransformIdx,
                AttributeAccessor{
                    .mBufferViewIndex = 0, // view is added above
                    .mClientDataFormat{
                        .mDimension = 1,
                        .mOffset = offsetof(InstanceData, mModelTransformIdx),
                        .mComponentType = GL_UNSIGNED_INT,
                    },
                }
            },
            {
                semantic::gMaterialIdx,
                AttributeAccessor{
                    .mBufferViewIndex = 0, // view is added above
                    .mClientDataFormat{
                        .mDimension = 1,
                        .mOffset = offsetof(InstanceData, mMaterialIdx),
                        .mComponentType = GL_UNSIGNED_INT,
                    },
                }
            },
        }
    };
}


} // namespace ad::renderer