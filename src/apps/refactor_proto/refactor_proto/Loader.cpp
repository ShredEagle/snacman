#include "Loader.h"

#include "BinaryArchive.h" 
#include "Json.h"
#include "Logging.h"

#include <arte/Image.h>

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
        assert(scale[0] == scale[1] && scale[0] == scale[2]);
        result.mUniformScale = scale[0];

        return result;
    }


    // TODO consolidate loading several attribute, several meshes into the same GL buffers
    // (the attribute interleaving might be done on the exporter side though).
    Part loadMesh(BinaryInArchive & aIn, Storage & aStorage, Material aDefaultMaterial, const GenericStream & aStream)
    {
        auto loadBuffer = [&aStorage, &aIn](GLsizei aElementSize, std::size_t aElementCount)
        {
            std::size_t bufferSize = aElementCount * aElementSize;
            std::unique_ptr<char[]> cpuBuffer = aIn.readBytes(bufferSize);

            auto [glBuffer, glBufferSize] = 
                makeMutableBuffer(std::span{cpuBuffer.get(), bufferSize}, GL_STATIC_DRAW);

            aStorage.mBuffers.push_back(std::move(glBuffer));

            return BufferView {
                .mGLBuffer = &aStorage.mBuffers.back(),
                .mStride = aElementSize,
                .mOffset = 0,
                .mSize = glBufferSize, // The view has access to the whole buffer
            };
        };

        unsigned int materialIndex;
        aIn.read(materialIndex);

        unsigned int verticesCount;
        aIn.read(verticesCount);
        constexpr auto gPositionSize = 3 * sizeof(float); // TODO that is crazy coupling
        BufferView positionView = loadBuffer(gPositionSize, verticesCount);

        constexpr auto gNormalSize = gPositionSize;
        BufferView normalView = loadBuffer(gNormalSize, verticesCount);

        unsigned int uvChannelsCount;
        aIn.read(uvChannelsCount);
        assert(uvChannelsCount <= 1);
        BufferView uvView;
        for(unsigned int uvIdx = 0; uvIdx != uvChannelsCount; ++uvIdx)
        {
            uvView = loadBuffer(2 * sizeof(float), verticesCount);
        }

        unsigned int primitiveCount;
        aIn.read(primitiveCount);
        const unsigned int indicesCount = 3 * primitiveCount;
        constexpr auto gIndexSize = sizeof(unsigned int);
        BufferView iboView = loadBuffer(gIndexSize, indicesCount);

        // TODO this is the stream we should reuse when storing several parts in the same buffer
        aStorage.mVertexStreams.push_back({
            .mVertexBufferViews{aStream.mVertexBufferViews},
            .mSemanticToAttribute{aStream.mSemanticToAttribute},
            .mIndexBufferView = iboView,
            .mIndicesType = GL_UNSIGNED_INT,
        });

        VertexStream & vertexStream = aStorage.mVertexStreams.back();

        vertexStream.mVertexBufferViews.insert(vertexStream.mVertexBufferViews.end(), { positionView, normalView});
        vertexStream.mSemanticToAttribute.insert({
            {
                semantic::gPosition,
                AttributeAccessor{
                    .mBufferViewIndex = vertexStream.mVertexBufferViews.size() - 2, // view is added above
                    .mClientDataFormat{
                        .mDimension = 3,
                        .mOffset = 0,
                        .mComponentType = GL_FLOAT
                    }
                }
            },
            {
                semantic::gNormal,
                AttributeAccessor{
                    .mBufferViewIndex = vertexStream.mVertexBufferViews.size() - 1, // view is added above
                    .mClientDataFormat{
                        .mDimension = 3,
                        .mOffset = 0,
                        .mComponentType = GL_FLOAT
                    }
                }
            },
        });

        if(uvChannelsCount != 0)
        {
            vertexStream.mSemanticToAttribute.emplace(
                semantic::gUv,
                AttributeAccessor{
                    // Size taken before pushing back
                    .mBufferViewIndex = vertexStream.mVertexBufferViews.size(),
                    .mClientDataFormat{
                        .mDimension = 2,
                        .mOffset = 0,
                        .mComponentType = GL_FLOAT,
                    },
                });
            vertexStream.mVertexBufferViews.push_back(std::move(uvView));
        }


        Part part{
            .mMaterial{
                .mPhongMaterialIdx = materialIndex,
                .mEffect = aDefaultMaterial.mEffect,
            },
            .mVertexStream = &vertexStream,
            .mPrimitiveMode = GL_TRIANGLES,
            .mVertexCount = (GLsizei)verticesCount,
            .mIndicesCount = (GLsizei)indicesCount,
        };

        aIn.read(part.mAabb);

        return part;
    }


    Object * addObject(Storage & aStorage)
    {
        aStorage.mObjects.emplace_back();
        return &aStorage.mObjects.back();
    }


    Node loadNode(BinaryInArchive & aIn, Storage & aStorage, const Material aDefaultMaterial, const GenericStream & aStream)
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
                loadMesh(aIn, aStorage, aDefaultMaterial, aStream)
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
            node.mChildren.push_back(loadNode(aIn, aStorage, aDefaultMaterial, aStream));
        }

        aIn.read(node.mAabb);

        return node;
    }


    graphics::Texture loadTexture(std::filesystem::path aTexturePath)
    {
        graphics::Texture texture{GL_TEXTURE_2D};
        arte::Image<math::sdr::Rgba> image{aTexturePath, arte::ImageOrientation::InvertVerticalAxis};
        graphics::loadImageCompleteMipmaps(texture, image);
        return texture;
    }


    // TODO Ad 2023/08/01: 
    // Review how the effects (the programs) are provided to the parts (currently hardcoded)
    void loadMaterials(BinaryInArchive & aIn, Storage & aStorage)
    {
        unsigned int materialsCount;
        aIn.read(materialsCount);

        std::vector<PhongMaterial> materials{materialsCount};
        aIn.read(std::span{materials});

        unsigned int pathsCount;
        aIn.read(pathsCount);
        for(unsigned int pathIdx = 0; pathIdx != pathsCount; ++pathIdx)
        {
            std::string path = aIn.readString();
            aStorage.mTextures.push_back(loadTexture(aIn.mParentPath / path));
        }

        aStorage.mPhongMaterials = std::move(materials);
    }


} // unnamed namespace


// TODO should not take a default material, but have a way to get the actual effect to use 
// (maybe directly from the binary file)
Node loadBinary(const std::filesystem::path & aBinaryFile, Storage & aStorage, Material aDefaultMaterial, const GenericStream & aStream)
{
    BinaryInArchive in{
        .mIn{std::ifstream{aBinaryFile, std::ios::binary}},
        .mParentPath{aBinaryFile.parent_path()},
    };

    // TODO we need a unified design where the load code does not duplicate the type of the write.
    unsigned int version;
    in.read(version);
    assert(version == 1);

    Node result = loadNode(in, aStorage, aDefaultMaterial, aStream);

    loadMaterials(in, aStorage);

    return result;
}


Effect * Loader::loadEffect(const std::filesystem::path & aEffectFile,
                            Storage & aStorage/*,
                            const FeatureSet & aFeatures*/)
{
    aStorage.mEffects.emplace_back();
    Effect & result = aStorage.mEffects.back();

    Json effect = Json::parse(std::ifstream{mFinder.pathFor(aEffectFile)});
    for (const auto & technique : effect.at("techniques"))
    {
        aStorage.mPrograms.push_back(ConfiguredProgram{
            .mProgram = loadProgram(technique.at("programfile")),
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


IntrospectProgram Loader::loadProgram(const filesystem::path & aProgFile)
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

    std::vector<std::string> defines;
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
            graphics::ShaderSource::Preprocess(*inputStream, defines, identifier, lookup));
    }

    SELOG(debug)("Compiling shader program from '{}', containing {} stages.", lookup.top(), shaders.size());

    return IntrospectProgram{
            graphics::makeLinkedProgram(shaders.begin(), shaders.end()),
            aProgFile.filename().string()
    };
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
                semantic::gLocalToWorld,
                AttributeAccessor{
                    .mBufferViewIndex = 0, // view is added above
                    .mClientDataFormat{
                        .mDimension = {4, 4},
                        .mOffset = offsetof(InstanceData, mModelTransform),
                        .mComponentType = GL_FLOAT
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