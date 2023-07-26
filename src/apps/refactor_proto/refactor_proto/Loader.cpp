#include "Loader.h"

#include "BinaryArchive.h" 
#include "Json.h"
#include "Logging.h"

#include <math/Homogeneous.h>

#include <renderer/BufferLoad.h>
#include <renderer/ShaderSource.h>
#include <renderer/utilities/FileLookup.h>


namespace ad::renderer {


struct InstanceData
{
    math::AffineMatrix<4, GLfloat> mModelTransform;
};


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
        // TODO enable GL 4.5+, here used for DSA
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


    VertexStream loadMesh(BinaryInArchive & aIn, Storage & aStorage)
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

        unsigned int verticesCount;
        aIn.read(verticesCount);
        constexpr auto gPositionSize = 3 * sizeof(float); // TODO that is crazy coupling
        BufferView positionView = loadBuffer(gPositionSize, verticesCount);

        constexpr auto gNormalSize = gPositionSize;
        BufferView normalView = loadBuffer(gNormalSize, verticesCount);

        unsigned int primitiveCount;
        aIn.read(primitiveCount);
        const unsigned int indicesCount = 3 * primitiveCount;
        constexpr auto gIndexSize = sizeof(unsigned int);
        BufferView iboView = loadBuffer(gIndexSize, indicesCount);

        return VertexStream{
            .mVertexBufferViews{ positionView, normalView},
            .mSemanticToAttribute{
                {
                    semantic::gPosition,
                    AttributeAccessor{
                        .mBufferViewIndex = 0, // view is added above
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
                        .mBufferViewIndex = 1, // view is added above
                        .mClientDataFormat{
                            .mDimension = 3,
                            .mOffset = 0,
                            .mComponentType = GL_FLOAT
                        }
                    }
                },
            },
            .mVertexCount = (GLsizei)verticesCount,
            .mPrimitiveMode = GL_TRIANGLES,
            .mIndexBufferView = iboView,
            .mIndicesType = GL_UNSIGNED_INT,
            .mIndicesCount = (GLsizei)indicesCount,
        };
    }

    Object * addObject(Storage & aStorage)
    {
        aStorage.mObjects.emplace_back();
        return &aStorage.mObjects.back();
    }

    Node loadNode(BinaryInArchive & aIn, Storage & aStorage, const Material aDefaultMaterial)
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
            node.mInstance.mObject->mParts.push_back(Part{
                .mVertexStream = loadMesh(aIn, aStorage),
                .mMaterial = aDefaultMaterial,
            });
        }

        for(std::size_t childIdx = 0; childIdx != childrenCount; ++childIdx)
        {
            node.mChildren.push_back(loadNode(aIn, aStorage, aDefaultMaterial));
        }

        return node;
    }


} // unnamed namespace


Node loadBinary(const std::filesystem::path & aBinaryFile, Storage & aStorage, Material aDefaultMaterial)
{
    BinaryInArchive in{
        .mIn{std::ifstream{aBinaryFile, std::ios::binary}},
    };

    // TODO we need a unified design where the load code does not duplicate the type of the write.
    unsigned int version;
    in.read(version);
    assert(version == 1);

    return loadNode(in, aStorage, aDefaultMaterial);
}


Effect * Loader::loadEffect(const std::filesystem::path & aEffectFile,
                            Storage & aStorage,
                            const FeatureSet & aFeatures)
{
    aStorage.mEffects.emplace_back();
    Effect & result = aStorage.mEffects.back();

    Json effect = Json::parse(std::ifstream{mFinder.pathFor(aEffectFile)});
    for (const auto & technique : effect.at("techniques"))
    {
        aStorage.mPrograms.push_back(loadProgram(technique.at("programfile")));
        result.mTechniques.push_back(
            Technique{
                .mProgram = &aStorage.mPrograms.back(),
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


VertexStream makeFromPositions(Storage & aStorage, std::span<math::Position<3, GLfloat>> aPositions)
{
    auto [vbo, size] = makeMutableBuffer(aPositions, GL_STATIC_DRAW);
    aStorage.mBuffers.push_back(std::move(vbo));

    BufferView vboView{
        .mGLBuffer = &aStorage.mBuffers.back(),
        .mStride = sizeof(decltype(aPositions)::value_type),
        .mOffset = 0,
        .mSize = size, // The view has access to the whole buffer
    };

    return VertexStream{
        .mVertexBufferViews{ vboView, },
        .mSemanticToAttribute{
            {
                semantic::gPosition,
                AttributeAccessor{
                    .mBufferViewIndex = 0, // view is added above
                    .mClientDataFormat{
                        .mDimension = 3,
                        .mOffset = 0,
                        .mComponentType = GL_FLOAT
                    }
                }
            },
        },
        .mVertexCount = (GLsizei)std::size(aPositions),
        .mPrimitiveMode = GL_TRIANGLES,
    };
}


SemanticBufferViews makeInstanceStream(Storage & aStorage, std::size_t aInstanceCount)
{
    auto [vbo, size] = makeMutableBuffer<InstanceData>(aInstanceCount, GL_STREAM_DRAW);
    aStorage.mBuffers.push_back(std::move(vbo));

    BufferView vboView{
        .mGLBuffer = &aStorage.mBuffers.back(),
        .mStride = sizeof(InstanceData),
        .mOffset = 0,
        .mSize = size, // The view has access to the whole buffer
    };

    return SemanticBufferViews{
        .mVertexBufferViews = { vboView, },
        .mSemanticToAttribute{
            {
                semantic::gLocalToWorld,
                AttributeAccessor{
                    .mBufferViewIndex = 0, // view is added above
                    .mClientDataFormat{
                        .mDimension = {4, 4},
                        .mOffset = 0,
                        .mComponentType = GL_FLOAT
                    },
                    .mInstanceDivisor = 1,
                }
            },
        }
    };
}


} // namespace ad::renderer