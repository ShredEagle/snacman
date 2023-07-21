#include "Loader.h"

#include "BinaryArchive.h" 

#include <math/Homogeneous.h>

#include <renderer/BufferLoad.h>


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


} // unnamed namespace


VertexStream loadBinary(Storage & aStorage, const std::filesystem::path & aBinaryFile)
{
    BinaryInArchive in{
        .mIn{std::ifstream{aBinaryFile, std::ios::binary}},
    };

    // TODO we need a unified design where the load code does not duplicate the type of the write.
    unsigned int version;
    in.read(version);
    assert(version == 1);

    unsigned int verticesCount;
    in.read(verticesCount);

    constexpr auto gPositionSize = 3 * sizeof(float); // TODO that is crazy coupling
    std::size_t positionBufferSize = verticesCount * gPositionSize;
    std::unique_ptr<char[]> positionBuffer = in.readBytes(positionBufferSize);

    auto [vbo, sizeVbo] = 
        makeMutableBuffer(std::span{positionBuffer.get(), positionBufferSize}, GL_STATIC_DRAW);

    aStorage.mBuffers.push_back(std::move(vbo));

    BufferView vboView{
        .mGLBuffer = &aStorage.mBuffers.back(),
        .mStride = gPositionSize,
        .mOffset = 0,
        .mSize = sizeVbo, // The view has access to the whole buffer
    };

    unsigned int primitiveCount;
    in.read(primitiveCount);
    const unsigned int indicesCount = 3 * primitiveCount;

    constexpr auto gIndexSize = 3 * sizeof(unsigned int);
    const std::size_t indexBufferSize = indicesCount * gIndexSize;
    std::unique_ptr<char[]> indexBuffer = in.readBytes(indexBufferSize);

    auto [ibo, sizeIbo] = 
        makeMutableBuffer(std::span{indexBuffer.get(), indexBufferSize}, GL_STATIC_DRAW);

    aStorage.mBuffers.push_back(std::move(ibo));

    BufferView iboView{
        .mGLBuffer = &aStorage.mBuffers.back(),
        .mOffset = 0,
        .mSize = sizeIbo, // The view has access to the whole buffer
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
        .mVertexCount = (GLsizei)verticesCount,
        .mPrimitiveMode = GL_TRIANGLES,
        .mIndexBufferView = iboView,
        .mIndicesType = GL_UNSIGNED_INT,
        .mIndicesCount = (GLsizei)indicesCount,
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
                semantic::gModelTransform,
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