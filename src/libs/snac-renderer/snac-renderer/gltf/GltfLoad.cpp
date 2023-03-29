#include "GltfLoad.h"

#include "LoadBuffer.h"

#include "../Logging.h"
#include "../Semantic.h"

#include <arte/gltf/Gltf.h>

#include <fmt/ostream.h>
#include <handy/StringUtilities.h>

#include <renderer/MappedGL.h>

#include <spdlog/spdlog.h>

#include <map>
#include <vector>

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
        MAPPING(TANGENT, Tangent)
        MAPPING(TEXCOORD_0, TextureCoords0)
        MAPPING(TEXCOORD_1, TextureCoords1)
        else
        {
            SELOG(debug)("Gltf semantic \"{}\" is not handled.", aGltfSemantic);
            return std::nullopt;
        }

        #undef MAPPING
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
                             aBufferView.id(), graphics::to_string(infered));
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
            graphics::to_string(target),
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

    enum ColorSpace
    {
        Linear,
        sRGB,
    };


    // Note: Might mutate the image, this is why it is taking it by value.
    template <class T_pixel>
    std::shared_ptr<graphics::Texture>
    prepareTexture(arte::Image<T_pixel> aImage,
                   const gltf::texture::Sampler & aSampler,
                   ColorSpace aSourceColorSpace)
    {
        auto result = std::make_shared<graphics::Texture>(GL_TEXTURE_2D);
        // Note: Alternatively to decoding the image to linear space on the CPU,
        // we might allocate texture storage with internal format GL_SRGB8_ALPHA8.
        // Yet "[OpenGL] implementations are allowed to perform this conversion after filtering,
        // [which] is inferior to converting from sRGB prior to filtering.""
        // (see: glspec4.6 core section 8.24)
        graphics::loadImageCompleteMipmaps(
            *result, 
            aSourceColorSpace == ColorSpace::sRGB ? decodeSRGBToLinear(aImage): aImage);

        // Sampling parameters
        {
            graphics::ScopedBind boundTexture{*result};

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, aSampler.wrapS);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, aSampler.wrapT);
            if (aSampler.magFilter)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, *aSampler.magFilter);
            }
            if (aSampler.minFilter)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, *aSampler.minFilter);
            }
        }

        return result;
    }


    template <class T_pixel>
    std::shared_ptr<graphics::Texture>
    prepareTexture(arte::Const_Owned<gltf::Texture> aTexture, ColorSpace aSourceColorSpace)
    {
        return prepareTexture(
            loadImageData<T_pixel>(checkImage(aTexture)),
            (aTexture->sampler ? 
                aTexture.get(&gltf::Texture::sampler) : gltf::texture::gDefaultSampler),
            aSourceColorSpace
        );
    }


    std::shared_ptr<graphics::Texture> makeDefaultNormalMap()
    {
        return prepareTexture(arte::Image<math::sdr::Rgb>{ {1, 1}, math::sdr::Rgb{128, 128, 255}},
                            gltf::texture::gDefaultSampler,
                            ColorSpace::Linear);
    }


    VertexStream prepareAttributes(Const_Owned<gltf::Primitive> aPrimitive)
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
                && vertexCount != (GLsizei)accessor->count)
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


    std::shared_ptr<Material> prepareMaterial(Const_Owned<gltf::Primitive> aPrimitive)
    {
        auto material = std::make_shared<Material>();

        if(aPrimitive->material)
        {
            auto gltfMaterial = aPrimitive.get(&gltf::Primitive::material);

            //
            // PBR Metallic Roughness
            //
            auto pbrMetallicRoughness =
                gltfMaterial->pbrMetallicRoughness.value_or(gltf::material::gDefaultPbr);
            material->mUniforms.mStore.emplace(Semantic::BaseColorFactor,
                                        UniformParameter{pbrMetallicRoughness.baseColorFactor});
            if(auto baseColorTexture = pbrMetallicRoughness.baseColorTexture)
            {
                material->mTextures.mStore.emplace(
                    Semantic::BaseColorTexture,
                    prepareTexture<math::sdr::Rgba>(
                        gltfMaterial.get<gltf::Texture>(baseColorTexture->index),
                        ColorSpace::sRGB));
                material->mUniforms.mStore.emplace(Semantic::BaseColorUVIndex,
                                            baseColorTexture->texCoord);
            }
            else
            {
                SELOG(error)
                     ("Gltf models are expected to have a base color texture for the moment.");
                throw std::runtime_error{"Gltf models are expected to have a base color texture for the moment."};
            }

            //
            // Normal texture
            //
            if(auto normalTexture = gltfMaterial->normalTexture)
            {
                material->mTextures.mStore.emplace(
                    Semantic::NormalTexture,
                    prepareTexture<math::sdr::Rgb>(
                        gltfMaterial.get<gltf::Texture>(normalTexture->index),
                        ColorSpace::Linear));
                material->mUniforms.mStore.emplace(Semantic::NormalUVIndex,
                                            normalTexture->texCoord);
                material->mUniforms.mStore.emplace(Semantic::NormalMapScale,
                                            normalTexture->scale);
            }
            else
            {
                static std::shared_ptr<graphics::Texture> gDefaultNormalMap = makeDefaultNormalMap();
                material->mTextures.mStore.emplace(Semantic::NormalTexture,
                                                   gDefaultNormalMap);
                material->mUniforms.mStore.emplace(Semantic::NormalUVIndex,
                                                   0u); // Arbitrary index, any UV coords will sample the same texel
                material->mUniforms.mStore.emplace(Semantic::NormalMapScale,
                                                   1.0f);
            }
        }

        return material;
    }


    Mesh makeFromPrimitive(Const_Owned<gltf::Primitive> aPrimitive)
    {
        return Mesh{
            .mStream = prepareAttributes(aPrimitive),
            .mMaterial = prepareMaterial(aPrimitive),
        };
    }

} // unnamed namespace


Model loadGltf(filesystem::path aModel, std::string_view aName)
{
    arte::Gltf gltf{aModel};

    Model model{
        .mName = std::string{aName},
    };

    // Collapse the primitives from all meshes in the file into a single collection of primitives.
    for (Owned<gltf::Mesh> gltfMesh : gltf.getMeshes())
    {
        const std::string & meshName = 
            (gltfMesh->name.empty() ? std::string{"<mesh#"} + std::to_string(gltfMesh.id()) + ">" : gltfMesh->name);
    
        for (Const_Owned<gltf::Primitive> gltfPrimitive : gltfMesh.iterate(&gltf::Mesh::primitives))
        {
            model.mParts.push_back(makeFromPrimitive(gltfPrimitive));
            model.mParts.back().mName = meshName + "_<prim#" + std::to_string(gltfPrimitive.id()) + ">";
        }

    }

    return model;
}


} // namespace snac
} // namespace ad
