#pragma once


#include "GraphicState_V2.h"
#include "Handle_V2.h"

// TODO Ad 2024/02/13: #RV2 Remove all V1 stuff
#include <snac-renderer-V1/Camera.h>
#include <snac-renderer-V1/DebugRenderer.h>
#include <snac-renderer-V1/LoadInterface.h>
#include <snac-renderer-V1/Mesh.h>
#include <snac-renderer-V1/Render.h>
#include <snac-renderer-V1/text/TextRenderer.h>
#include <snac-renderer-V1/UniformParameters.h>
#include <snac-renderer-V1/pipelines/ForwardShadows.h>

// V2: the good stuff
#include <snac-renderer-V2/Model.h>
#include <snac-renderer-V2/Pass.h>
#include <snac-renderer-V2/VertexStreamUtilities.h>
#include <snac-renderer-V2/files/Loader.h>

#include <filesystem>
#include <memory>


namespace ad {
namespace arte { class FontFace; }
namespace graphics { class AppInterface; }

// Forward declarations
namespace snac {
    class Resources;
    struct Font;
} // namespace snac

namespace snacgame {


namespace semantic {
    #define SEM(s) const renderer::Semantic g ## s{#s}
    #define BLOCK_SEM(s) const renderer::BlockSemantic g ## s{#s}

    SEM(LocalToWorld);
    SEM(Albedo);
    SEM(EntityIdx);
    SEM(MaterialIdx);
    SEM(MatrixPaletteOffset);

    BLOCK_SEM(ViewProjection);
    BLOCK_SEM(Materials);
    BLOCK_SEM(JointMatrices);
    BLOCK_SEM(Entities);
    BLOCK_SEM(Lighting);

    #undef SEMANTIC
} // namespace semantic


// The Render Graph for snacman
struct SnacGraph
{
    /// @brief Model for the per game-entity data that has to be available in shaders.
    ///
    /// An entity is not matching the notion of GL instance (since an entity maps to an Object, wich might contains several Parts).
    /// So this cannot be made available as instance attributes, but should probably be accessed via buffer-backed blocks.
    struct alignas(16) EntityData
    {
        // Note: 16-aligned, because it is intended to be stored as an array in a buffer object
        // and then the elements are accessed via a std140 uniform block 

        math::AffineMatrix<4, GLfloat> mModelTransform;
        // TODO Do we use an entity-level color?
        //math::sdr::Rgba mAlbedo;
        GLuint mMatrixPaletteOffset; // offset to the first joint of this instance in the buffer of joints.
    };
    
    static_assert(sizeof(EntityData) % 16 == 0, 
        "Size a multiple of 16 so successive loads work as expected.");

    struct alignas(16) LightingData
    {
        alignas(16) math::hdr::Rgb<GLfloat> mAmbientColor;
        alignas(16) math::Position<3, GLfloat>   mLightPosition_c;
        alignas(16) math::hdr::Rgb<GLfloat> mLightColor;
    };

    /// @brief The data to populate the GL instance attribute buffer, for instanced rendering.
    ///
    /// From our data model, the graphics API instance is the Part (not the Object)
    struct InstanceData
    {
        GLuint mEntityIdx;
        GLuint mMaterialIdx; 
    };

    /// @brief The list of parts to be rendered. This might be valid for several passes (intra, or inter frame)
    struct PartList
    {
        void push_back(const renderer::Part * aPart, const renderer::Material * aMaterial, GLuint aInstanceIdx)
        {
            mParts.push_back(aPart);
            mMaterials.push_back(aMaterial);
            mInstanceIdx.push_back(aInstanceIdx);
        }

        // SOA
        // TODO replace pointers with handles? There is not really a handle for parts though
        std::vector<const renderer::Part *> mParts;
        // If the entity UBO is pre-populated with material indices, we might only need the effect.
        std::vector<const renderer::Material *> mMaterials;
        // Index of this part in the instance buffer, which is filled before creating the partlist.
        std::vector<GLuint> mInstanceIdx;
        // Alternatively, might also store the EntityIdx, and create a sorted Instance stream per pass...
        // This would more easily allow to do instanced rendering with multidraw, at the cost of loading the instance buffer each pass.
        //std::vector<GLuint> mEntityIdx;
    };

    static renderer::PassCache preparePass(renderer::StringKey aPass,
                                           const PartList & aPartList,
                                           renderer::Storage & aStorage);

    static renderer::GenericStream makeInstanceStream(renderer::Storage & aStorage, std::size_t aInstanceCount)
    {
        renderer::BufferView vboView = renderer::makeBufferGetView(sizeof(InstanceData),
                                                                   aInstanceCount,
                                                                   1, // intance divisor
                                                                   GL_STREAM_DRAW,
                                                                   aStorage);

        // TODO Ad 2024/04/04: #perf Indices should be grouped by 4 in vertex attributes,
        // instead of using a distinct attribute each.
        return renderer::GenericStream{
            .mVertexBufferViews = { vboView, },
            .mSemanticToAttribute{
                {
                    semantic::gEntityIdx,
                    renderer::AttributeAccessor{
                        .mBufferViewIndex = 0, // view is added above
                        .mClientDataFormat{
                            .mDimension = 1,
                            .mOffset = offsetof(InstanceData, mEntityIdx),
                            .mComponentType = GL_UNSIGNED_INT,
                        },
                    }
                },
                {
                    semantic::gMaterialIdx,
                    renderer::AttributeAccessor{
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

    // TODO #RV2: This class should be implementing the frame rendering of models, instead of 
    // having it inline in Renderer_V2::render()
    //void renderFrame()

    // List and describes the buffer views used for per-instance vertex attributes
    renderer::GenericStream mInstanceStream;

    graphics::Buffer<graphics::BufferType::DrawIndirect> mIndirectBuffer;

    struct GraphUbos
    {
        // TODO should they be hosted in renderer::Storage instead of this class?
        graphics::UniformBufferObject mJointMatricesUbo;
        graphics::UniformBufferObject mEntitiesUbo;
        graphics::UniformBufferObject mLightingUbo;

        renderer::RepositoryUbo mUboRepository{
            {semantic::gJointMatrices, &mJointMatricesUbo},
            {semantic::gEntities, &mEntitiesUbo},
            {semantic::gLighting, &mLightingUbo},
        };
    } mGraphUbos;

};

using Resources_V2 = renderer::Loader;

// TODO Ad 2024/02/13: #RV2 Let it organically emerge, then move that type to a dedicated file
struct Impl_V2
{
    // TODO Ad 2024/02/14: We need a better approach:
    // * Dynamic buffer size?
    // * Something clever?
    static constexpr std::size_t gMaxInstances = 1024;

    renderer::Storage mStorage;
    SnacGraph mRenderGraph{
        .mInstanceStream = SnacGraph::makeInstanceStream(mStorage, gMaxInstances),
    };

};


class Renderer_V2
{
    friend struct TextRenderer;

    struct Control
    {
        MovableAtomic<bool> mRenderModels{true};
        MovableAtomic<bool> mRenderText{true};
        MovableAtomic<bool> mRenderDebug{true};

        // This boolean is only accessed by main thread
        bool mShowShadowControls{false};
    };

public:
    using GraphicState_t = visu_V2::GraphicState;

    using Resources_t = Resources_V2;

    // This is smelly, ideally we should not Handle alias a second time.
    // But is currently used by RenderThread to derive the handle type from the renderer type.
    template <class T_resource>
    using Handle_t = renderer::Handle<T_resource>;

    Renderer_V2(graphics::AppInterface & aAppInterface,
                snac::Load<snac::Technique> & aTechniqueAccess,
                arte::FontFace aDebugFontFace);

    //void resetProjection(float aAspectRatio, snac::Camera::Parameters aParameters);

    renderer::Handle<const renderer::Object> loadModel(filesystem::path aModel,
                                                       filesystem::path aEffect,
                                                       Resources_t & aResources);

    // TODO Ad 2024/03/28: #font #RV2 still using the V1 renderer (and V1 type of handle)
    std::shared_ptr<snac::Font> loadFont(arte::FontFace aFontFace,
                                         unsigned int aPixelHeight,
                                         filesystem::path aEffect,
                                         snac::Resources & aResources);

    void continueGui();

    void render(const GraphicState_t & aState);

    /// \brief Forwards the request to reset repositories down to the generic renderer.
    void resetRepositories()
    { mRendererToDecomission.resetRepositories(); }

private:
    template <class T_range>
    void renderText(const T_range & aTexts, snac::ProgramSetup & aProgramSetup);

    Control mControl;
    graphics::AppInterface & mAppInterface;
    snac::Renderer mRendererToDecomission; // TODO #RV2 Remove this data member
    Impl_V2 mRendererToKeep;
    // TODO Is it the correct place to host the pipeline instance?
    // This notably force to instantiate it with the Renderer (before the Resources manager is available).
    snac::ForwardShadows mPipelineShadows;
    snac::Camera mCamera;
    snac::CameraBuffer mCameraBuffer;
    snac::TextRenderer mTextRenderer;
    snac::GlyphInstanceStream mDynamicStrings;
    snac::DebugRenderer mDebugRenderer;

    // Intended for function-local storage, made a member so its reuses the allocated memory between frames.
    std::vector<math::AffineMatrix<4, GLfloat>> mRiggingPalettesBuffer;
    std::vector<SnacGraph::InstanceData> mInstanceBuffer;
};


} // namespace snacgame
} // namespace ad