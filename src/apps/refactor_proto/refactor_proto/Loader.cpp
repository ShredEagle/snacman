#include "Loader.h"

#include "BinaryArchive.h" 
#include "Json.h"
#include "Logging.h"

#include <arte/Image.h>
// TODO Ad 2023/10/06: Move the Json <-> Math type converters to a better place
#include <arte/detail/GltfJson.h>

#include <renderer/BufferLoad.h>
#include <renderer/ShaderSource.h>
#include <renderer/Texture.h>
#include <renderer/utilities/FileLookup.h>


namespace ad::renderer {


namespace {

    template <class T_data>
    std::pair<graphics::BufferAny, GLsizeiptr>
    makeMutableBuffer(std::size_t aInstanceCount, GLenum aHint)
    {
        graphics::BufferAny buffer; // glGenBuffers()
        // Note: Using a random target, the underlying buffer objects are all identical.
        constexpr auto target = graphics::BufferType::Array;
        graphics::ScopedBind boundBuffer{buffer, target}; // glBind()
        const GLsizeiptr size = sizeof(T_data) * aInstanceCount;
        glBufferData(
            static_cast<GLenum>(target),
            size,
            nullptr,
            aHint);
        return std::make_pair(std::move(buffer), size);
    }


    template <class T_data, std::size_t N_extent>
    std::pair<graphics::BufferAny, GLsizeiptr>
    makeMutableBuffer(std::span<T_data, N_extent> aInitialData, GLenum aHint)
    {
        graphics::BufferAny buffer; // glGenBuffers()
        // Note: Using a random target, the underlying buffer objects are all identical.
        constexpr auto target = graphics::BufferType::Array;
        graphics::ScopedBind boundBuffer{buffer, target}; // glBind()
        const GLsizeiptr size = aInitialData.size_bytes();
        // TODO enable GL 4.5+, so we could use DSA here
        glBufferData(
            static_cast<GLenum>(target),
            size,
            aInitialData.data(),
            aHint);
        return std::make_pair(std::move(buffer), size);
    }

    template <std::floating_point T_value>
    bool isWithinTolerance(T_value aLhs, T_value aRhs, T_value aRelativeTolerance)
    {
        T_value diff = std::abs(aLhs - aRhs);
        T_value maxMagnitude = std::max(std::abs(aLhs), std::abs(aRhs));
        return diff <= (maxMagnitude * aRelativeTolerance);
    }

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
    BufferView createBuffer(GLsizei aElementSize, GLsizei aElementCount, Storage & aStorage)
    {
        graphics::BufferAny glBuffer; // glGenBuffers()
        // Note: Using a random target, the underlying buffer objects are all identical.
        constexpr auto target = graphics::BufferType::Array;
        graphics::ScopedBind boundBuffer{glBuffer, target}; // glBind()

        const GLsizeiptr bufferSize = aElementSize * aElementCount;

        glBufferData(
            static_cast<GLenum>(target),
            bufferSize,
            nullptr,
            GL_STATIC_DRAW);

        aStorage.mBuffers.push_back(std::move(glBuffer));
        graphics::BufferAny * buffer = &aStorage.mBuffers.back();

        return BufferView{
            .mGLBuffer = buffer,
            .mStride = aElementSize,
            .mOffset = 0,
            .mSize = bufferSize, // The view has access to the whole buffer ATM
        };
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

    // Provide a distinct buffer for each attribute stream at the moment (i.e. no interleaving).
    VertexStream * prepareConsolidatedStream(unsigned int aVerticesCount,
                                             unsigned int aIndicesCount,
                                             std::span<const AttributeDescription> aBufferedStreams,
                                             Storage & aStorage,
                                             const GenericStream & aStream)
    {
        // TODO Ad 2023/10/11: Should we support smaller index types.
        using Index_t = GLuint;
        BufferView iboView = createBuffer(sizeof(Index_t), aIndicesCount, aStorage);

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
            BufferView attributeView = createBuffer(attributeSize, aVerticesCount, aStorage);

            vertexStream.mSemanticToAttribute.emplace(
                attribute.mSemantic,
                AttributeAccessor{
                    .mBufferViewIndex = vertexStream.mVertexBufferViews.size(), // view is added next
                    .mClientDataFormat{
                        .mDimension = attribute.mDimension,
                        .mOffset = 0,
                        .mComponentType = GL_FLOAT
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
        auto loadBuffer = [&aIn](graphics::BufferAny & aBuffer, 
                                 GLuint aElementFirst,
                                 GLsizei aElementSize,
                                 std::size_t aElementCount)
        {
            std::size_t bufferSize = aElementCount * aElementSize;
            std::unique_ptr<char[]> cpuBuffer = aIn.readBytes(bufferSize);

            // Note: Using a random target, the underlying buffer objects are all identical.
            constexpr auto target = graphics::BufferType::Array;
            graphics::ScopedBind boundBuffer{aBuffer, target}; // glBind()

            glBufferSubData(
                static_cast<GLenum>(target),
                aElementFirst * aElementSize, // offset
                bufferSize, // data length
                cpuBuffer.get());
        };

        // TODO #loader Those hardcoded indices are smelly, hard-coupled to the attribute streams structure in the binary.
        graphics::BufferAny & positionBuffer = *(aVertexStream.mVertexBufferViews.end() - 4)->mGLBuffer;
        graphics::BufferAny & normalBuffer = *(aVertexStream.mVertexBufferViews.end() - 3)->mGLBuffer;
        graphics::BufferAny & colorBuffer = *(aVertexStream.mVertexBufferViews.end() - 2)->mGLBuffer;
        graphics::BufferAny & uvBuffer = *(aVertexStream.mVertexBufferViews.end() - 1)->mGLBuffer;
        graphics::BufferAny & indexBuffer = *(aVertexStream.mIndexBufferView.mGLBuffer);

        unsigned int materialIndex;
        aIn.read(materialIndex);

        unsigned int verticesCount;
        aIn.read(verticesCount);
        
        // load positions
        loadBuffer(positionBuffer, aVertexFirst, gPositionSize, verticesCount);
        // load normals
        loadBuffer(normalBuffer, aVertexFirst, gNormalSize, verticesCount);

        unsigned int colorChannelsCount;
        aIn.read(colorChannelsCount);
        assert(colorChannelsCount <= 1);
        for(unsigned int colorIdx = 0; colorIdx != colorChannelsCount; ++colorIdx)
        {
            loadBuffer(colorBuffer, aVertexFirst, gColorSize, verticesCount);
        }

        unsigned int uvChannelsCount;
        aIn.read(uvChannelsCount);
        assert(uvChannelsCount <= 1);
        
        // TODO handle no UV channel by adding all of them at the end? (so it is not problem we index out of range)
        assert(uvChannelsCount == 1);

        for(unsigned int uvIdx = 0; uvIdx != uvChannelsCount; ++uvIdx)
        {
            loadBuffer(uvBuffer, aVertexFirst, gUvSize, verticesCount);
        }

        unsigned int primitiveCount;
        aIn.read(primitiveCount);
        const unsigned int indicesCount = 3 * primitiveCount;
        loadBuffer(indexBuffer, aIndexFirst, gIndexSize, indicesCount);

        // Assign the part material index to the otherwise common material
        aPartsMaterial.mPhongMaterialIdx = materialIndex;
        Part part{
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
        unsigned int meshesCount, childrenCount;
        aIn.read(meshesCount);
        aIn.read(childrenCount);

        math::Matrix<4, 3, GLfloat> localTransformation;
        aIn.read(localTransformation);

        Node node{
            .mInstance{
                .mObject = (meshesCount > 0) ? addObject(aStorage) : nullptr,
                .mPose = decompose(math::AffineMatrix<4, GLfloat>{localTransformation}),
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

        graphics::writeTo(a3dTexture,
                          static_cast<const std::byte *>(image),
                          graphics::InputImageParameters::From(image),
                          math::Position<3, GLint>{0, 0, aLayerIdx});
    }


    // TODO Ad 2023/08/01: 
    // Review how the effects (the programs) are provided to the parts (currently hardcoded)
    void loadMaterials(BinaryInArchive & aIn,
                       graphics::UniformBufferObject & aDestinationUbo,
                       graphics::Texture & aDestinationTexture)
    {
        {
            unsigned int materialsCount;
            aIn.read(materialsCount);

            std::vector<PhongMaterial> materials{materialsCount};
            aIn.read(std::span{materials});

            graphics::load(aDestinationUbo, std::span{materials}, graphics::BufferHint::StaticDraw);
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
            glTexStorage3D(textureArray.mTarget, 
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
    // TODO actually read the correct buffer size (i.e. write it first)
    //constexpr GLsizei gBufferSize = 512 * 1024 * 1024;
    VertexStream * consolidatedStream = prepareConsolidatedStream(verticesCount,
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
        .mContext = &aStorage.mMaterialContexts.back(),
        .mEffect = aPartsEffect,
    };

    GLuint vertexFirst = 0;
    GLuint indexFirst = 0;
    Node result = loadNode(in, aStorage, partsMaterial, *consolidatedStream, vertexFirst, indexFirst);

    loadMaterials(in, aStorage.mUbos.back(), aStorage.mTextures.back());

    return result;
}


Effect * Loader::loadEffect(const std::filesystem::path & aEffectFile,
                            Storage & aStorage,
                            const std::vector<std::string> & aDefines_temp)
{
    aStorage.mEffects.emplace_back();
    Effect & result = aStorage.mEffects.back();

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

    return IntrospectProgram{
            graphics::makeLinkedProgram(shaders.begin(), shaders.end()),
            aProgFile.filename().string()
    };
}


Scene Loader::loadScene(const filesystem::path & aSceneFile,
                        const filesystem::path & aEffectFile,
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
                                loadEffect(aEffectFile, aStorage, defines),
                                aStream);

        // TODO store and load names in the binary file format
        SELOG(info)("Loaded model '{}' with bouding box {}.",
                     modelFile.stem().string(), fmt::streamed(model.mAabb));

        Pose pose;
        if(entry.contains("pose"))
        {
            pose.mPosition = entry.at("pose").value<math::Vec<3, float>>("position", {});
            pose.mUniformScale = entry.at("pose").value("uniform_scale", 1.f);

            
        }

        Node modelPoser{
            .mInstance = Instance{
                .mPose = pose,
            },
            .mChildren = {std::move(model)},
        };
        scene.addToRoot(std::move(modelPoser));
    }

    return scene;
}


//VertexStream makeFromPositions(Storage & aStorage, std::span<math::Position<3, GLfloat>> aPositions)
//{
//    auto [vbo, size] = makeMutableBuffer(aPositions, GL_STATIC_DRAW);
//    aStorage.mBuffers.push_back(std::move(vbo));
//
//    BufferView vboView{
//        .mGLBuffer = &aStorage.mBuffers.back(),
//        .mStride = sizeof(decltype(aPositions)::value_type),
//        .mOffset = 0,
//        .mSize = size, // The view has access to the whole buffer
//    };
//
//    return VertexStream{
//        .mVertexBufferViews{ vboView, },
//        .mSemanticToAttribute{
//            {
//                semantic::gPosition,
//                AttributeAccessor{
//                    .mBufferViewIndex = 0, // view is added above
//                    .mClientDataFormat{
//                        .mDimension = 3,
//                        .mOffset = 0,
//                        .mComponentType = GL_FLOAT
//                    }
//                }
//            },
//        },
//        .mVertexCount = (GLsizei)std::size(aPositions),
//        .mPrimitiveMode = GL_TRIANGLES,
//    };
//}


GenericStream makeInstanceStream(Storage & aStorage, std::size_t aInstanceCount)
{
    auto [vbo, size] = makeMutableBuffer<InstanceData>(aInstanceCount, GL_STREAM_DRAW);
    aStorage.mBuffers.push_back(std::move(vbo));

    BufferView vboView{
        .mGLBuffer = &aStorage.mBuffers.back(),
        .mStride = sizeof(InstanceData),
        .mInstanceDivisor = 1,
        .mOffset = 0,
        .mSize = size, // The view has access to the whole buffer
    };

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