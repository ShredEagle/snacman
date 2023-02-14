#include "GltfLoad.h"

#include "LoadBuffer.h"

#include "../Logging.h"
#include "../Semantic.h"

#include <arte/gltf/Gltf.h>

#include <map>

using namespace ad::arte;


#define SELOG(level) SELOG_LG(gGltfLogger, level)


namespace ad {
namespace snac {

namespace {

    std::optional<Semantic> translateGltfSemantic(std::string_view aGltfSemantic)
    {
        #define MAPPING(gltfstring, semantic) \
            else if(aGltfSemantic == #gltfstring) { return Semantic::semantic; }

        if(false){}
        MAPPING(POSITION, Position)
        MAPPING(NORMAL, Normal)
        else
        {
            SELOG(debug)("Gltf semantic \"{}\" is not handled.", aGltfSemantic);
            return std::nullopt;
        }
    }


    template <class T_buffer>
    T_buffer prepareBuffer(Const_Owned<gltf::Accessor> aAccessor,
                           Const_Owned<gltf::BufferView> aBufferView)
    {
        GLenum target = [&]()
        {
            if (!aBufferView->target)
            {
                const GLenum infered = T_buffer::GLTarget_v;
                SELOG(trace)("Buffer view #{} does not have target defined. Infering {}.",
                             aBufferView.id(), infered);
                return infered;
            }
            else
            {
                assert(*aBufferView->target == T_buffer::GLTarget_v);
                return *aBufferView->target;
            }
        }();

        T_buffer buffer;

        glBindBuffer(target, buffer);
        glBufferData(target,
                     aBufferView->byteLength,
                     loadBufferData(aAccessor, aBufferView).data(),
                     GL_STATIC_DRAW);
        glBindBuffer(target, 0);

        SELOG(debug)
            ("Loaded {} bytes in target {}, offset in source buffer is {} bytes.",
            aBufferView->byteLength,
            target,
            aBufferView->byteOffset);

        return buffer;
    }


    BufferView prepareBufferView(Const_Owned<gltf::Accessor> aAccessor)
    {
        auto gltfBufferView = checkedBufferView(aAccessor);
        return BufferView{
            .mBuffer = prepareBuffer<graphics::VertexBufferObject>(aAccessor, gltfBufferView),
            .mStride = (GLsizei)gltfBufferView->byteStride.value_or(0),
        };
    }


    graphics::ClientAttribute attributeFromAccessor(const gltf::Accessor & aAccessor)
    {
        return graphics::ClientAttribute{
            .mDimension = getAccessorDimension(aAccessor.type),
            .mOffset = aAccessor.byteOffset,
            .mComponentType = aAccessor.componentType,
        };
    }


    math::Box<GLfloat> getBoundingBox(Const_Owned<gltf::Accessor> aAccessor, Semantic aSemantic)
    {
        assert(aSemantic == Semantic::Position);

        if (!aAccessor->bounds)
        {
            SELOG(critical)
                ("Accessor \"{}\" with semantic {} does not define bounds.",
                aAccessor.id(), to_string(aSemantic));
            throw std::logic_error{"Accessor MUST have bounds."};
        }
        // By the spec, position MUST be a VEC3 of float.
        auto & bounds = std::get<gltf::Accessor::MinMax<float>>(*aAccessor->bounds);

        math::Position<3, GLfloat> min{bounds.min[0], bounds.min[1], bounds.min[2]};
        math::Position<3, GLfloat> max{bounds.max[0], bounds.max[1], bounds.max[2]};
        return math::Box<GLfloat>{
            min,
            (max - min).as<math::Size>(),
        };
    }


    VertexStream makeFromPrimitive(Const_Owned<gltf::Primitive> aPrimitive)
    {
        VertexStream stream;

        stream.mPrimitive = aPrimitive->mode;

        GLsizei vertexCount = std::numeric_limits<GLsizei>::max();
        std::map<gltf::Index<gltf::BufferView>::Value_t, std::size_t/*buffer view index*/> bufferViewMap;

        for(const auto & [gltfSemantic, accessorId] : aPrimitive->attributes)
        {
            SELOG(debug)("Semantic \"{}\" is associated to accessor #{}", gltfSemantic, accessorId);

            Const_Owned<gltf::Accessor> accessor = aPrimitive.get(accessorId);

            if(vertexCount != std::numeric_limits<GLsizei>::max() 
                && vertexCount != accessor->count)
            {
                SELOG(critical)
                    ("Semantic \"{}\", accessor #{} has a count of {}, while previous accessors where {}.",
                    gltfSemantic, accessorId, accessor->count, vertexCount);
                throw std::runtime_error{"Inconsistent accessors count."};
            }
            vertexCount = (GLsizei)accessor->count;

            if(!accessor->bufferView)
            {
                SELOG(warn)("Skipping semantic \"{}\", associated to accessor #{} which does not have a buffer view.", gltfSemantic, accessorId);
                continue;
            }

            std::size_t bufferViewId;
            auto gltfBufferViewId = accessor->bufferView->value;
            if(auto found = bufferViewMap.find(gltfBufferViewId);
                found != bufferViewMap.end())
            {
                SELOG(trace)
                    ("Semantic \"{}\", associated to accessor #{}, reuses mesh buffer view #{}.",
                    gltfSemantic, accessorId, found->second);
                bufferViewId = found->second;
            }
            else
            {
                // TODO implement some assertions that distinct buffer views do not overlap on a buffer range.
                // (since at the moment we create one distinct GL buffer per buffer view).
                bufferViewId = stream.mVertexBuffers.size();
                stream.mVertexBuffers.push_back(prepareBufferView(accessor));
                bufferViewMap.emplace(gltfBufferViewId, bufferViewId);
            }

            if (auto semantic = translateGltfSemantic(gltfSemantic))
            {
                stream.mAttributes.emplace(
                    *semantic,
                    AttributeAccessor{
                        .mBufferViewIndex = bufferViewId,
                        .mAttribute = attributeFromAccessor(accessor),
                    }
                );

                if(semantic == Semantic::Position)
                {
                    stream.mBoundingBox = getBoundingBox(accessor, *semantic);
                    SELOG(debug)
                        ("Mesh primitive #{} has bounding box {}.",
                         aPrimitive.id(), fmt::streamed(stream.mBoundingBox));
                }
            }
            else
            {
                SELOG(warn)("Skipping semantic \"{}\", associated to accessor #{}, which does not have a translation.", gltfSemantic, accessorId);
            }
        }

        if(aPrimitive->indices)
        {
            SELOG(debug)("Primitive #{} defines indexed geometry.", aPrimitive.id());
            Const_Owned<gltf::Accessor> indicesAccessor = 
                aPrimitive.get(&gltf::Primitive::indices);

            if(bufferViewMap.find(indicesAccessor->bufferView->value)
                != bufferViewMap.end())
            {
                // This would be suboptimal, meaning we load two buffers with the same data
                SELOG(error)
                    ("Primitive #{} indices reuses a buffer view used for vertex attributes.",
                    aPrimitive.id());
            }

            auto gltfBufferView = checkedBufferView(indicesAccessor);
            stream.mIndices = IndicesAccessor{
                .mIndexBuffer = prepareBuffer<graphics::IndexBufferObject>(indicesAccessor, gltfBufferView),
                .mIndexCount = (GLsizei)indicesAccessor->count,
                .mAttribute = attributeFromAccessor(indicesAccessor),
            };
        }

        assert(vertexCount != std::numeric_limits<GLsizei>::max());
        stream.mVertexCount = vertexCount;
        return stream;
    }
} // unnamed namespace


VertexStream loadGltf(filesystem::path aModel)
{
    arte::Gltf gltf{aModel};

    // TODO this should be extended, to allow picking sub-meshes from a Gltf file
    // and even support multiple primitives in a Mesh (but the Mesh class will need redesigning) 

    // Get first mesh
    Owned<gltf::Mesh> gltfMesh = gltf.get(gltf::Index<gltf::Mesh>{0});
    // Load the first (and only) primitive
    if(gltfMesh->primitives.size() != 1)
    {
        SELOG(error)
            ("The gltf model '{}' first mesh has {} primitives, exactly 1 is required.",
            aModel.string(), gltfMesh->primitives.size());
        throw std::runtime_error{"Gltf file without a single primitive on the first mesh."};
    }

    return makeFromPrimitive({gltf, gltfMesh->primitives.at(0), 0});
}


} // namespace snac
} // namespace ad