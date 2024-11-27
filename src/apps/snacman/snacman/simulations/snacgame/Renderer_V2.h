#pragma once


#include "GraphicState_V2.h"
#include "Handle_V2.h"

#include <handy/AtomicVariations.h>

// TODO Ad 2024/02/13: #RV2 Remove all V1 stuff
#include <snac-renderer-V1/Render.h>
#include <snac-renderer-V1/text/TextRenderer.h>

// V2: the good stuff
#include <snac-renderer-V2/Camera.h>
#include <snac-renderer-V2/Model.h>
#include <snac-renderer-V2/Pass.h>
#include <snac-renderer-V2/Semantics.h>

#include <snac-renderer-V2/debug/DebugRenderer.h>

#include <snac-renderer-V2/files/Loader.h>

#include <snac-renderer-V2/graph/ShadowMapping.h>

#include <snac-renderer-V2/utilities/VertexStreamUtilities.h>

#include <filesystem>
#include <memory>


namespace ad {

namespace arte { class FontFace; }
namespace graphics { class AppInterface; }
namespace renderer {
    struct Font;
} // namespace renderer

// Forward declarations
namespace snac {
    class Resources;
} // namespace snac



namespace snacgame {


// The Render Graph for snacman
struct SnacGraph
{
    /// @brief Model for the per game-entity data that has to be available in shaders.
    ///
    /// An entity is not matching the notion of GL instance (since an entity maps to an Object, wich might contains several Parts).
    /// So this cannot be made available as instance attributes, but should probably be accessed via buffer-backed blocks.
    struct alignas(16) EntityData_glsl
    {
        // Note: 16-aligned, because it is intended to be stored as an array in a buffer object
        // and then the elements are accessed via a std140 uniform block

        math::AffineMatrix<4, GLfloat> mModelTransform;
        math::hdr::Rgba_f mColorFactor;
    };

    static_assert(sizeof(EntityData_glsl) % 16 == 0,
        "Size a multiple of 16 so successive loads work as expected.");

    /// @brief The data to populate the GL instance attribute buffer, for instanced rendering.
    ///
    /// From our data model, the graphics API instance is the Part (not the Object)
    struct InstanceData
    {
        GLuint mEntityIdx;
        GLuint mMaterialParametersIdx;
        GLuint mMatrixPaletteOffset = (GLuint)-1;
    };

    /// @brief The list of parts to be rendered. This might be valid for several passes (intra, or inter frame)
    struct PartList : public renderer::PartList
    {
        void push_back(const renderer::Part * aPart, const renderer::Material * aMaterial, GLuint aInstanceIdx)
        {
            mParts.push_back(aPart);
            mMaterials.push_back(aMaterial);
            mInstanceIdx.push_back(aInstanceIdx);
        }

        // SOA, continued
        // Index of this part in the instance buffer, which is filled before creating the partlist.
        std::vector<GLuint> mInstanceIdx;
        // Alternatively, might also store the EntityIdx, and create a sorted Instance stream per pass...
        // This would more easily allow to do instanced rendering with multidraw, at the cost of loading the instance buffer each pass.
        //std::vector<GLuint> mEntityIdx;
    };

    static renderer::PassCache preparePass(std::vector<renderer::Technique::Annotation> aAnnotations,
                                           const PartList & aPartList,
                                           renderer::Storage & aStorage);

    static renderer::GenericStream makeInstanceStream(renderer::Storage & aStorage)
    {
        renderer::BufferView vboView = renderer::makeBufferGetView(sizeof(InstanceData),
                                                                   0, // Do not allocate storage space
                                                                   1, // intance divisor
                                                                   GL_STREAM_DRAW,
                                                                   aStorage);

        // TODO Ad 2024/04/04: #perf Indices should be grouped by 4 in vertex attributes,
        // instead of using a distinct attribute each.
        return renderer::GenericStream{
            .mVertexBufferViews = { vboView, },
            .mSemanticToAttribute{
                {
                    renderer::semantic::gEntityIdx,
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
                    renderer::semantic::gMaterialIdx,
                    renderer::AttributeAccessor{
                        .mBufferViewIndex = 0, // view is added above
                        .mClientDataFormat{
                            .mDimension = 1,
                            .mOffset = offsetof(InstanceData, mMaterialParametersIdx),
                            .mComponentType = GL_UNSIGNED_INT,
                        },
                    }
                },
                {
                    renderer::semantic::gMatrixPaletteOffset,
                    renderer::AttributeAccessor{
                        .mBufferViewIndex = 0, // view is added above
                        .mClientDataFormat{
                            .mDimension = 1,
                            .mOffset = offsetof(InstanceData, mMatrixPaletteOffset),
                            .mComponentType = GL_UNSIGNED_INT,
                        },
                    }
                },
            }
        };
    }

    /// @brief Non-multi, direct, instanced drawing approach.
    /// @param aObject The object that will be rendered (each part via a separated instanced draw call)
    /// @param aBaseInstance The base instance in the current instance buffer.
    /// @param aInstancesCount Number of instances, for each Part of the Object.
    void drawInstancedDirect(const renderer::Object & aObject,
                             unsigned int & aBaseInstance,
                             const GLsizei aInstancesCount,
                             renderer::StringKey aPass,
                             renderer::Storage & aStorage);

    void renderWorld(const visu_V2::GraphicState & aState,
                     renderer::Storage & aStorage,
                     math::Size<2, int> aFramebufferSize);

    void renderDebugFrame(const snac::DebugDrawer::DrawList & aDrawList, renderer::Storage & aStorage);

    void renderScreenText(const visu_V2::GraphicState & aState,
                          renderer::Storage & aStorage,
                          math::Size<2, int> aFramebufferSize);

    void passDepth(SnacGraph::PartList aPartList,
                   renderer::RepositoryTexture aTextureRepository,
                   renderer::Storage & aStorage,
                   renderer::DepthMethod aDepthMethod);

    static constexpr bool gMultiIndirectDraw = true;

    // List and describes the buffer views used for per-instance vertex attributes
    renderer::GenericStream mInstanceStream;

    graphics::Buffer<graphics::BufferType::DrawIndirect> mIndirectBuffer;

    // TODO find a more descriptive name
    struct GraphUbos
    {
        // TODO should they be hosted in renderer::Storage instead of this class?
        graphics::UniformBufferObject mJointMatricesUbo;
        graphics::UniformBufferObject mEntitiesUbo;
        graphics::UniformBufferObject mLightsUbo;
        graphics::UniformBufferObject mLightViewProjectionUbo;
        graphics::UniformBufferObject mShadowCascadeUbo;
        graphics::UniformBufferObject mViewingProjectionUbo;

        renderer::RepositoryUbo mUboRepository{
            {renderer::semantic::gJointMatrices, &mJointMatricesUbo},
            {renderer::semantic::gEntities, &mEntitiesUbo},
            {renderer::semantic::gLights, &mLightsUbo},
            {renderer::semantic::gLightViewProjection, &mLightViewProjectionUbo},
            {renderer::semantic::gShadowCascade, &mShadowCascadeUbo},
            {renderer::semantic::gViewProjection, &mViewingProjectionUbo},
        };
    } mGraphUbos;

    // Intended for function-local storage, made a member so its reuses the allocated memory between frames.
    std::vector<math::AffineMatrix<4, GLfloat>> mRiggingPalettesBuffer;
    std::vector<SnacGraph::InstanceData> mInstanceBuffer;

    renderer::ShadowMapping mShadowMapping;

    renderer::DebugRenderer mDebugRenderer;
};

using Resources_V2 = renderer::Loader;

// TODO Ad 2024/02/13: #RV2 Let it organically emerge, then move that type to a dedicated file
// TODO Ad 2024/04/11: Why did I add this separate class? Is it of any use?
struct Impl_V2
{
    Impl_V2(const renderer::Loader & aLoader) :
        mRenderGraph{
            .mInstanceStream = SnacGraph::makeInstanceStream(mStorage),
            .mShadowMapping = renderer::ShadowMapping{mStorage},
            .mDebugRenderer = renderer::DebugRenderer{mStorage, aLoader},
        }
    {
        // TODO use cascades
        mRenderGraph.mShadowMapping.mControls.mUseCascades = true;
        mRenderGraph.mShadowMapping.mControls.mCsmNearPlaneLimit = -5.f;
        mRenderGraph.mShadowMapping.mControls.mSlopeScale = 3.f;
        mRenderGraph.mShadowMapping.mControls.mUnitScale = 100.f;
        // Because at the moment we can only debug draw from main (game) thread
        mRenderGraph.mShadowMapping.mControls.turnOffDebugDrawing();
    }

    renderer::Storage mStorage;
    SnacGraph mRenderGraph;
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
                resource::ResourceFinder & aResourcesFinder,
                arte::FontFace aDebugFontFace);

    //void resetProjection(float aAspectRatio, snac::Camera::Parameters aParameters);

    renderer::Handle<const renderer::Object> loadModel(filesystem::path aModel,
                                                       filesystem::path aEffect,
                                                       Resources_t & aResources);

    // No, changing that now
    //// TODO Ad 2024/03/28: #font #RV2 still using the V1 renderer (and V1 type of handle)
    std::shared_ptr<FontAndPart> loadFont(arte::FontFace aFontFace,
                                          unsigned int aPixelHeight,
                                          snac::Resources & aResources);

    void continueGui();

    void render(const GraphicState_t & aState);

    /// \brief Forwards the request to reset repositories down to the generic renderer.
    void resetRepositories()
    { mRendererToDecomission.resetRepositories(); }

    void recompileEffectsV2();

private:
    Control mControl;
    graphics::AppInterface & mAppInterface;
    snac::Renderer mRendererToDecomission; // TODO #RV2 Remove this data member
    // TODO #resource_redesign
    // This is saved as a member because the loader is needed to recompile effects
    renderer::Loader mLoader;
    Impl_V2 mRendererToKeep;
    snac::TextRenderer mTextRenderer;
    snac::GlyphInstanceStream mDynamicStrings;
};


} // namespace snacgame
} // namespace ad
