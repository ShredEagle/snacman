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
                SELOG(info)("Buffer view #{} does not have target defined. Infering {}.",
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
                     // TODO might be even better to only load in main memory the part of the buffer starting
                     // at bufferView->byteOffset (and also limit the length there, actually).
                     loadBufferData(aAccessor, aBufferView).data() 
                         + aBufferView->byteOffset,
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


    Mesh makeFromPrimitive(Const_Owned<gltf::Primitive> aPrimitive)
    {
        Mesh mesh;

        mesh.mStream.mPrimitive = aPrimitive->mode;

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

            if(!accessor->bufferView)
            {
                SELOG(error)("Skipping semantic \"{}\", associated to accessor #{} which does not have a buffer view.", gltfSemantic, accessorId);
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
                bufferViewId = mesh.mStream.mVertexBuffers.size();
                mesh.mStream.mVertexBuffers.push_back(prepareBufferView(accessor));
                bufferViewMap.emplace(gltfBufferViewId, bufferViewId);
            }

            if (auto semantic = translateGltfSemantic(gltfSemantic))
            {
                mesh.mStream.mAttributes.emplace(
                    *semantic,
                    AttributeAccessor{
                        .mBufferViewIndex = bufferViewId,
                        .mAttribute = {
                            .mDimension = getAccessorDimension(accessor->type),
                            .mOffset = accessor->byteOffset,
                            .mComponentType = accessor->componentType,
                        }
                    }
                );
            }
            else
            {
                SELOG(warn)("Skipping semantic \"{}\", associated to accessor #{}, which does not have a translation.", gltfSemantic, accessorId);
            }
        }

        mesh.mStream.mVertexCount = vertexCount;
        return mesh;
    }
};

    // Scene iteration logic
    //auto scene = gltf.getDefaultScene();
    //if(!scene)
    //{
    //    SELOG(error)("The gltf model '{}' does not define a default scene.", aModel);
    //    throw std::runtime_error{"Gltf file without a default scene (not handled)."};
    //}
    //for(auto node : scene->iterate(&arte::gltf::Scene::nodes))
    //{
    //    
    //}

Mesh loadGltf(filesystem::path aModel)
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