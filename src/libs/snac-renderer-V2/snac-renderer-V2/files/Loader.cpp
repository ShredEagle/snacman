#include "Loader.h"

#include "BinaryArchive.h" 
#include "Flags.h" 
#include "Versioning.h" 

#include "../Cube.h"
#include "../Json.h"
#include "../Logging.h"
#include "../RendererReimplement.h"
#include "../Rigging.h"
#include "../Semantics.h"
#include "../VertexStreamUtilities.h"

#include <arte/Image.h>

#include <math/Transformations.h>

#include <renderer/BufferLoad.h>
#include <renderer/ShaderSource.h>
#include <renderer/Texture.h>
#include <renderer/utilities/FileLookup.h>

#include <cassert>


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

    // TODO #math #linearalgebra move to a more generic library (graphics?)
    Pose decompose(const math::AffineMatrix<4, GLfloat> & aTransformation)
    {
        // TODO Ad 2024/03/27: #pose Also get the rotation part!
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

        assert(isWithinTolerance(scale[1], scale[0], 1E-6f));
        assert(isWithinTolerance(scale[2], scale[0], 1E-6f));
        //assert(scale[0] == scale[1] && scale[0] == scale[2]);
        result.mUniformScale = scale[0];

        return result;
    }

    template <class T_value>
    T_value readAs(BinaryInArchive & aIn)
    {
        T_value v;
        aIn.read(v);
        return v;
    }

    template <class T_element>
    std::vector<T_element> readAsVector(BinaryInArchive & aIn,
                                        std::size_t aElementCount,
                                        T_element aDefaultValue = {})
    {
        std::vector<T_element> result(aElementCount, aDefaultValue);
        aIn.read(std::span{result});
        return result;
    }


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
    constexpr auto gTangentSize = gPositionSize;
    constexpr auto gBitangentSize = gPositionSize;
    constexpr auto gUvSize = 2 * sizeof(float);
    constexpr auto gColorSize = 4 * sizeof(float);
    constexpr auto gIndexSize = sizeof(unsigned int);
    constexpr auto gJointDataSize = sizeof(VertexJointData);

    // TODO Do we want attribute interleaving, or are we content with one distinct buffer per attribute ?
    // (the attribute interleaving should then be done on the exporter side).
    Part loadMesh(BinaryInArchive & aIn, 
                  const VertexStream & aVertexStream,
                  // TODO #inout replace this inout by something more sane
                  GLuint & aVertexFirst,
                  GLuint & aIndexFirst,
                  Material aPartsMaterial /*makes a copy, to edit for this specific part*/)
    {
        // Load data into an existing buffer, with offset aElementFirst.
        auto loadBufferFromArchive = 
            [&aIn](const graphics::BufferAny & aBuffer, 
                   GLuint aElementFirst,
                   GLsizei aElementSize,
                   std::size_t aElementCount)
            {
                std::size_t bufferSize = aElementCount * aElementSize;
                std::unique_ptr<char[]> cpuBuffer = aIn.readBytes(bufferSize);
                loadBuffer(aBuffer, aElementFirst * aElementSize, std::span{cpuBuffer.get(), bufferSize});
            };

        // TODO #loader Those hardcoded indices are smelly, hard-coupled to the attribute streams structure in the binary.
        const graphics::BufferAny & positionBuffer  = *(aVertexStream.mVertexBufferViews.end() - 7)->mGLBuffer;
        const graphics::BufferAny & normalBuffer    = *(aVertexStream.mVertexBufferViews.end() - 6)->mGLBuffer;
        const graphics::BufferAny & tangentBuffer   = *(aVertexStream.mVertexBufferViews.end() - 5)->mGLBuffer;
        const graphics::BufferAny & bitangentBuffer = *(aVertexStream.mVertexBufferViews.end() - 4)->mGLBuffer;
        const graphics::BufferAny & colorBuffer     = *(aVertexStream.mVertexBufferViews.end() - 3)->mGLBuffer;
        const graphics::BufferAny & uvBuffer        = *(aVertexStream.mVertexBufferViews.end() - 2)->mGLBuffer;
        const graphics::BufferAny & jointDataBuffer = *(aVertexStream.mVertexBufferViews.end() - 1)->mGLBuffer;
        const graphics::BufferAny & indexBuffer     = *(aVertexStream.mIndexBufferView.mGLBuffer);

        const std::string meshName = aIn.readString();

        unsigned int materialIndex;
        aIn.read(materialIndex);

        unsigned int vertexAttributesFlags;
        aIn.read(vertexAttributesFlags);
        assert(has(vertexAttributesFlags, gVertexPosition));
        assert(has(vertexAttributesFlags, gVertexNormal));

        unsigned int verticesCount;
        aIn.read(verticesCount);
        
        // load positions
        loadBufferFromArchive(positionBuffer, aVertexFirst, gPositionSize, verticesCount);
        // load normals
        loadBufferFromArchive(normalBuffer, aVertexFirst, gNormalSize, verticesCount);
        // load tangents & bitangents, if present
        // #assetprocessor This assert should go away if we start supporting dynamic list of attributes
        assert(has(vertexAttributesFlags, gVertexTangent));
        // Has to be true atm
        if(has(vertexAttributesFlags, gVertexTangent)) 
        {
            loadBufferFromArchive(tangentBuffer, aVertexFirst, gTangentSize, verticesCount);
        }
        assert(has(vertexAttributesFlags, gVertexBitangent));
        // Has to be true atm
        if(has(vertexAttributesFlags, gVertexBitangent)) 
        {
            loadBufferFromArchive(bitangentBuffer, aVertexFirst, gBitangentSize, verticesCount);
        }

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

        {
            unsigned int bonesCount;
            aIn.read(bonesCount);

            if(bonesCount != 0)
            {
                loadBufferFromArchive(jointDataBuffer, aVertexFirst, gJointDataSize, verticesCount);
            }
        }

        // Assign the part material index to the otherwise common material
        aPartsMaterial.mMaterialParametersIdx = materialIndex;
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


    Handle<Object> addObject(Storage & aStorage)
    {
        aStorage.mObjects.emplace_back();
        return &aStorage.mObjects.back();
    }


    Node loadNode(BinaryInArchive & aIn,
                  Storage & aStorage,
                  const Material & aBaseMaterial, // Common values are set, not the specific phong material index (it is set by each part)
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

        Handle<Object> object = [&]()
        {
            Handle<Object> result = (meshesCount > 0) ? addObject(aStorage) : gNullHandle;

            for(std::size_t meshIdx = 0; meshIdx != meshesCount; ++meshIdx)
            {
                result->mParts.push_back(
                    loadMesh(aIn, aVertexStream, aVertexFirst, aIndexFirst, aBaseMaterial)
                );
            }

            return result;
        }();

        Node node{
            .mInstance{
                .mObject = object,
                .mPose = decompose(math::AffineMatrix<4, GLfloat>{localTransformation}),
                .mName = std::move(name),
            },
        };

        
        // Rig of the Node
        unsigned int rigCount;
        aIn.read(rigCount);
        // Only one rig at most for the moment
        assert(rigCount <= 1);
        if(rigCount == 1)
        {
            // A Rig is assigned to an Object
            assert(node.mInstance.mObject != nullptr);

            // Joint tree
            unsigned int treeEntries;
            aIn.read(treeEntries);

            Rig rig{
                .mJointTree = NodeTree<Rig::Pose>{
                    .mHierarchy  = readAsVector<NodeTree<Rig::Pose>::Node>(aIn, treeEntries),
                    .mFirstRoot  = readAs<NodeTree<Rig::Pose>::Node::Index>(aIn),
                    .mLocalPose  = readAsVector<Rig::Pose>(aIn, treeEntries, Rig::Pose::Identity()),
                    .mGlobalPose = readAsVector<Rig::Pose>(aIn, treeEntries, Rig::Pose::Identity()),
                    .mNodeNames  = readAs<decltype(NodeTree<Rig::Pose>::mNodeNames)>(aIn),
                }
            };

            // Joint data
            {
                unsigned int jointCount;
                aIn.read(jointCount);
                rig.mJoints = Rig::JointData{
                    .mIndices = readAsVector<NodeTree<Rig::Pose>::Node::Index>(aIn, jointCount),
                    .mInverseBindMatrices = readAsVector<Rig::Ibm>(aIn, jointCount, Rig::Ibm::Identity()),
                };
            }

            // Name
            rig.mArmatureName = aIn.readString();

            aStorage.mAnimatedRigs.push_back({.mRig = std::move(rig)});
            object->mAnimatedRig = &aStorage.mAnimatedRigs.back();
        }


        math::Box<float> objectAabb;
        aIn.read(objectAabb);
        if(node.mInstance.mObject != nullptr)
        {
            object->mAabb = objectAabb;
        }

        for(std::size_t childIdx = 0; childIdx != childrenCount; ++childIdx)
        {
            node.mChildren.push_back(loadNode(aIn, aStorage, aBaseMaterial, aVertexStream, aVertexFirst, aIndexFirst));
        }

        aIn.read(node.mAabb);

        return node;
    }


    std::vector<RigAnimation> loadAnimations(BinaryInArchive & aIn)
    {
        auto animationsCount = readAs<unsigned int>(aIn);

        std::vector<RigAnimation> result;
        result.reserve(animationsCount);

        for(unsigned int animIdx = 0; animIdx != animationsCount; ++animIdx)
        {
            RigAnimation & rigAnimation = result.emplace_back(RigAnimation{
                .mName = aIn.readString(),
                .mDuration = readAs<float>(aIn),
            });

            auto keyframesCount = readAs<unsigned int>(aIn);
            rigAnimation.mTimepoints = readAsVector<float>(aIn, keyframesCount);

            auto nodesCount = readAs<unsigned int>(aIn);
            rigAnimation.mNodes = readAsVector<NodeTree<Rig::Pose>::Node::Index>(aIn, nodesCount);

            for(unsigned int nodeIdx = 0; nodeIdx != nodesCount; ++nodeIdx)
            {
                rigAnimation.mKeyframes.emplace_back(RigAnimation::NodeKeyframes{
                    .mTranslations = readAsVector<math::Vec<3, float>>(aIn, keyframesCount),
                    .mRotations = readAsVector<math::Quaternion<float>>(
                        aIn, keyframesCount, math::Quaternion<float>::Identity()),
                    .mScales = readAsVector<math::Vec<3, float>>(aIn, keyframesCount),
                });
            }
        }

        return result;
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
    MaterialContext loadMaterials(BinaryInArchive & aIn, Storage & aStorage)
    {
        unsigned int materialsCount;
        aIn.read(materialsCount);

        graphics::UniformBufferObject & ubo = aStorage.mUbos.emplace_back();
        {
            std::vector<GenericMaterial> materials{materialsCount};
            aIn.read(std::span{materials});

            graphics::load(ubo, std::span{materials}, graphics::BufferHint::StaticDraw);
        }

        { // load material names
            unsigned int namesCount; // TODO: useless, but this is because the filewritter wrote the number of elements in a vector
            aIn.read(namesCount);
            assert(namesCount == materialsCount);

            auto & materialNames = aStorage.mMaterialNames;
            materialNames.reserve(materialNames.size() + materialsCount);
            for(unsigned int nameIdx = 0; nameIdx != materialsCount; ++nameIdx)
            {
                materialNames.push_back(aIn.readString());
            }
        }

        auto loadTextures = [&aIn, &aStorage]() -> graphics::Texture *
        {
            math::Size<2, int> imageSize;        
            aIn.read(imageSize);

            unsigned int pathsCount;
            aIn.read(pathsCount);

            if(pathsCount > 0)
            {
                assert(imageSize.width() > 0 && imageSize.height() > 1);

                graphics::Texture & textureArray = aStorage.mTextures.emplace_back(GL_TEXTURE_2D_ARRAY);
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

                return &textureArray;
            }
            else
            {
                return nullptr;
            }
        };

        // IMPORTANT: This sequence is in the exact order of texture types in a binary.
        static const Semantic semanticSequence[] = {
            semantic::gDiffuseTexture,
            semantic::gNormalTexture,
            semantic::gMetallicRoughnessAoTexture,
        };
        RepositoryTexture textureRepo;
        for (Semantic texSemantic : semanticSequence)
        {
            if(graphics::Texture * texture = loadTextures())
            {
                textureRepo.emplace(texSemantic, texture);
            }
        }

        return MaterialContext{
            .mUboRepo = {{semantic::gMaterials, &ubo}},
            .mTextureRepo = std::move(textureRepo),
        };
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

    using IndexType = GLuint;

    //
    // Populate the vertex attributes values, client side
    //
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
    std::vector<IndexType> triangleIndices{0, 1, 2};

    auto cubePositions = getExpandedCubePositions();
    auto cubeNormals = getExpandedCubeNormals();
    std::vector<math::hdr::Rgba_f> cubeColors(cubePositions.size(), math::hdr::gWhite<GLfloat>);
    assert(cubePositions.size() == cubeNormals.size());

    //TODO handle non-indexed geometry in draw(), so we can get rid of the sequential indices.
    std::vector<IndexType> cubeIndices(cubePositions.size());
    std::iota(cubeIndices.begin(), cubeIndices.end(), 0);

    const unsigned int verticesCount = (unsigned int)(trianglePositions.size() + cubePositions.size());
    const unsigned int indicesCount = (unsigned int)(triangleIndices.size() + cubeIndices.size());

    //
    // Prepare the buffers and vertex streams
    // Note: The buffers are shared by both shapes,
    //       but we want each shape to use distinct BufferViews (so distinct VertexStream).
    //       This tests BufferView offsets.

    // The buffers are for both shapes, so we use the cumulative vertices and indices counts
    Handle<graphics::BufferAny> indexBuffer = makeBuffer(
        sizeof(IndexType),
        indicesCount,
        GL_STATIC_DRAW,
        aStorage
    );

    std::vector<Handle<graphics::BufferAny>> attributeBuffers;
    for(auto attribute : gAttributeStreamsInBinary)
    {
        attributeBuffers.push_back(
            makeBuffer(
                getByteSize(attribute),
                verticesCount,
                GL_STATIC_DRAW,
                aStorage
            ));
    }

    // The vertex streams are separate
    Handle<VertexStream> triangleStream = primeVertexStream(aStorage, aStream);
    setIndexBuffer(triangleStream, 
                   graphics::MappedGL_v<IndexType>,
                   indexBuffer,
                   (GLuint)triangleIndices.size(),
                   0/*offset*/);
    Handle<VertexStream> cubeStream = primeVertexStream(aStorage, aStream);
    setIndexBuffer(cubeStream,
                   graphics::MappedGL_v<IndexType>,
                   indexBuffer,
                   (GLuint)cubeIndices.size(),
                   sizeof(IndexType) * triangleIndices.size());

    for(unsigned int idx = 0; idx != gAttributeStreamsInBinary.size(); ++idx)
    {
        const auto & attribute = gAttributeStreamsInBinary[idx];
        const auto & buffer = attributeBuffers[idx];

        addVertexAttribute(triangleStream, attribute, buffer, (GLuint)trianglePositions.size(), 0/*offset*/);
        addVertexAttribute(cubeStream,     attribute, buffer, (GLuint)cubePositions.size(),     getByteSize(attribute) * trianglePositions.size());
    }

    //
    // Load data into the buffers of the vertex streams
    //
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
    // Push the objects in storage, return their Nodes.
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


// TODO should not take a default effect, but have a way to get the actual effect to use 
// (maybe directly from the binary file)
std::variant<Node, SeumErrorCode> loadBinary(const std::filesystem::path & aBinaryFile,
                                             Storage & aStorage,
                                             Effect * aPartsEffect,
                                             const GenericStream & aStream)
{
    if(aBinaryFile.extension() != ".seum")
    {
        SELOG(error)("Files with extension '{}' are not supported.",
                     aBinaryFile.extension().string());
        return SeumErrorCode::UnsupportedFormat;
    }

    BinaryInArchive in{
        .mIn{std::ifstream{aBinaryFile, std::ios::binary}},
        .mParentPath{aBinaryFile.parent_path()},
    };

    // TODO we need a unified design where the load code does not duplicate the type of the write.
    std::uint16_t magicValue{0};
    in.read(magicValue);
    if(magicValue != gSeumMagic)
    {
        SELOG(error)("The seum magic preamble is not matching.");
        return SeumErrorCode::InvalidMagicPreamble;
    }

    unsigned int version;
    in.read(version);
    assert(version <= gSeumVersion);
    if(version != gSeumVersion)
    {
        SELOG(warn)("The provided binary has version {}, but current version of the format is {}.", 
                    version, gSeumVersion);
        return SeumErrorCode::OutdatedVersion;
    }

    unsigned int verticesCount = 0;
    unsigned int indicesCount = 0;
    in.read(verticesCount);
    in.read(indicesCount);


    // Binary attributes descriptions
    // TODO Ad 2023/10/11: #loader Support dynamic set of attributes in the binary
    static const std::array<AttributeDescription, 6> gAttributeStreamsInBinary {
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
            .mSemantic = semantic::gTangent,
            .mDimension = 3,
            .mComponentType = GL_FLOAT,
        },
        AttributeDescription{
            .mSemantic = semantic::gBitangent,
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

    static const std::array<std::pair<AttributeDescription, std::size_t>, 2> gInterleavedJointAttributes {{
        {
            AttributeDescription{
                .mSemantic = semantic::gJoints0,
                .mDimension = VertexJointData::gMaxBones,
                .mComponentType = GL_UNSIGNED_INT,
            },
            offsetof(VertexJointData, mBoneIndices),
        },
        {
            AttributeDescription{
                .mSemantic = semantic::gWeights0,
                .mDimension = VertexJointData::gMaxBones,
                .mComponentType = GL_FLOAT,
            },
            offsetof(VertexJointData, mBoneWeights),
        }
    }};

    // Prepare the single buffer storage for the whole binary
    VertexStream * consolidatedStream = makeVertexStream(
        verticesCount,
        indicesCount,
        GL_UNSIGNED_INT,
        gAttributeStreamsInBinary,
        aStorage,
        aStream);

    // Add the interleaved joint attributes to the vertex stream
    Handle<graphics::BufferAny> vertexJointDataBuffer = makeBuffer(
        sizeof(VertexJointData),
        verticesCount,
        GL_STATIC_DRAW,
        aStorage);
    addInterleavedAttributes(
        consolidatedStream,
        sizeof(VertexJointData),
        gInterleavedJointAttributes,
        vertexJointDataBuffer,
        verticesCount);

    // Prepare a MaterialContext for the whole binary, 
    // I.e. in a scene, each entry (each distinct binary) gets its own material UBO and its own Texture array.
    // TODO #azdo #perf we could load textures and materials in a single buffer for a whole scene to further consolidate
    MaterialContext & commonMaterialContext = aStorage.mMaterialContexts.emplace_back();
    
    // Set the common values shared by all parts in this binary.
    // (The actual phong material index will be set later, by each part)
    Material baseMaterial{
        // The material names is a single array for all binaries
        // the binary-local material index needs to be offset to index into the common name array.
        .mNameArrayOffset = aStorage.mMaterialNames.size(),
        .mContext = &commonMaterialContext,
        .mEffect = aPartsEffect,
    };

    GLuint vertexFirst = 0;
    GLuint indexFirst = 0;

    // TODO Ad 2024/03/07: Find a cleaner (more direct and explicit) way to handle rigs and their animations
    // Currently compares the number of AnimatedRigs before and after loading then node, to see if one was added
    std::size_t rigsCount = aStorage.mAnimatedRigs.size();
    Node result = loadNode(in, aStorage, baseMaterial, *consolidatedStream, vertexFirst, indexFirst);
    rigsCount = aStorage.mAnimatedRigs.size() - rigsCount;

    // Hackish way to add the RigAnimations into the AnimateRig
    {
        std::vector<RigAnimation> animations = loadAnimations(in);
        
        if(animations.empty())
        {
            assert(rigsCount == 0);
        }
        else
        {
            assert(rigsCount == 1);
            auto & nameToAnim = aStorage.mAnimatedRigs.back().mNameToAnimation;
            for(RigAnimation & anim : animations)
            {
                nameToAnim.emplace(anim.mName, std::move(anim));
            }
        }
    }
    

    // TODO #assetprocessor If materials were loaded first, then the empty context 
    // Then we could directly instantiate the MaterialContext with this value.
    commonMaterialContext = loadMaterials(in, aStorage);

    return result;
}


// todo move in uns or private member of Loader
std::vector<Technique> populateTechniques(const Loader & aLoader,
                                          const filesystem::path & aEffectPath,
                                          Storage & aStorage,
                                          std::vector<std::string> aDefines)
{
    std::vector<Technique> result;
    Json effect = Json::parse(std::ifstream{aEffectPath});

    for (const auto & technique : effect.at("techniques"))
    {
        // Merge the potential defines found at the technique level into aDefines
        if(auto techniqueDefines = technique.find("defines");
           techniqueDefines != technique.end())
        {
            aDefines.insert(aDefines.end(),
                            techniqueDefines->begin(),
                            techniqueDefines->end());
        }

        result.push_back(Technique{
            .mConfiguredProgram =
                storeConfiguredProgram(aLoader.loadProgram(technique.at("programfile"), aDefines), aStorage),
        });

        if(technique.contains("annotations"))
        {
            Technique & inserted = result.back();
            for(auto [category, value] : technique.at("annotations").items())
            {
                inserted.mAnnotations.emplace(category, value.get<std::string>());
            }
        }
    }

    return result;
}


bool recompileEffects(const Loader & aLoader, Storage & aStorage)
{
    assert(aStorage.mEffectLoadInfo.size() == aStorage.mEffects.size());

    bool success = true;

    //for(unsigned int effectIdx = 0; effectIdx != aStorage.mEffectPaths.size(); ++effectIdx)
    auto effectIt = aStorage.mEffects.begin();
    for(const auto & loadInfo : aStorage.mEffectLoadInfo)
    {
        try
        {
            if(loadInfo != Storage::gNullLoadInfo)
            {
                effectIt->mTechniques = populateTechniques(aLoader, loadInfo.mPath, aStorage, loadInfo.mDefines);
            }
        }
        catch(const std::exception& aException)
        {
            SELOG(error)
            ("Exception thrown while compiling techniques:\n{}", aException.what());
            success = false;
        }
        
        ++effectIt;
    }

    return success;
}


Effect * Loader::loadEffect(const std::filesystem::path & aEffectFile,
                            Storage & aStorage,
                            const std::vector<std::string> & aDefines_temp) const
{
    aStorage.mEffects.emplace_back();
    Effect & result = aStorage.mEffects.back();
    result.mName = aEffectFile.string();

    filesystem::path effectPath = mFinder.pathFor(aEffectFile);
    aStorage.mEffectLoadInfo.push_back({
        .mPath = effectPath,
        .mDefines = aDefines_temp,
    });

    result.mTechniques = populateTechniques(*this, effectPath, aStorage, aDefines_temp);

    return &result;
}


IntrospectProgram Loader::loadProgram(const filesystem::path & aProgFile,
                                      std::vector<std::string> aDefines_temp) const
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
        if(shaderStage == "defines")
        {
            // Special case, not a shader stage
            for (std::string macro : shaderFile)
            {
                aDefines_temp.push_back(std::move(macro));
            }
            SELOG(warn)("'{}' program has 'defines', which are not recommended with V2.", aProgFile.string());
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


graphics::ShaderSource Loader::loadShader(const filesystem::path & aShaderFile) const
{
    return graphics::ShaderSource::Preprocess(mFinder.pathFor(aShaderFile));
}


GenericStream makeInstanceStream(Storage & aStorage, std::size_t aInstanceCount)
{
    BufferView vboView = makeBufferGetView(sizeof(InstanceData),
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
                        .mOffset = offsetof(InstanceData, mMaterialParametersIdx),
                        .mComponentType = GL_UNSIGNED_INT,
                    },
                }
            },
            {
                semantic::gMatrixPaletteOffset,
                AttributeAccessor{
                    .mBufferViewIndex = 0, // view is added above
                    .mClientDataFormat{
                        .mDimension = 1,
                        .mOffset = offsetof(InstanceData, mMatrixPaletteOffset),
                        .mComponentType = GL_UNSIGNED_INT,
                    },
                }
            }
        }
    };
}


} // namespace ad::renderer