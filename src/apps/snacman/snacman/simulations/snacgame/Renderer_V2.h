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
    SEM(MaterialIdx);

    BLOCK_SEM(ViewProjection);
    BLOCK_SEM(Materials);
    BLOCK_SEM(JointMatrices);

    #undef SEMANTIC
} // namespace semantic


// The Render Graph for snacman
struct SnacGraph
{
    struct InstanceData
    {
        math::AffineMatrix<4, GLfloat> mModelTransform;
        math::sdr::Rgba mAlbedo;
        GLuint mMaterialIdx;
    };

    static renderer::GenericStream makeInstanceStream(renderer::Storage & aStorage, std::size_t aInstanceCount)
    {
        renderer::BufferView vboView = renderer::makeBufferGetView(sizeof(InstanceData),
                                                                   aInstanceCount,
                                                                   1, // intance divisor
                                                                   GL_STREAM_DRAW,
                                                                   aStorage);

        return renderer::GenericStream{
            .mVertexBufferViews = { vboView, },
            .mSemanticToAttribute{
                {
                    semantic::gLocalToWorld,
                    renderer::AttributeAccessor{
                        .mBufferViewIndex = 0, // view is added above
                        .mClientDataFormat{
                            .mDimension = {4, 4},
                            .mOffset = offsetof(InstanceData, mModelTransform),
                            .mComponentType = GL_FLOAT,
                        },
                    }
                },
                {
                    semantic::gAlbedo,
                    renderer::AttributeAccessor{
                        .mBufferViewIndex = 0, // view is added above
                        .mClientDataFormat{
                            .mDimension = 4,
                            .mOffset = offsetof(InstanceData, mAlbedo),
                            .mComponentType = GL_UNSIGNED_BYTE,
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

    // TODO #RV2: It should be implementing the frame rendering of models, instead of 
    // having it inline in Renderer_V2::render()
    //void renderFrame()
    // List and describes the buffer views used for per-instance vertex attributes
    renderer::GenericStream mInstanceStream;
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
    using Handle_t = snacgame::Handle<T_resource>;

    Renderer_V2(graphics::AppInterface & aAppInterface,
                snac::Load<snac::Technique> & aTechniqueAccess,
                arte::FontFace aDebugFontFace);

    //void resetProjection(float aAspectRatio, snac::Camera::Parameters aParameters);

    Handle<renderer::Node> loadModel(filesystem::path aModel,
                                     filesystem::path aEffect,
                                     Resources_t & aResources);

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

    // Intended for local storage, made a member so its reuses the allocated memory between frames.
    std::vector<math::AffineMatrix<4, GLfloat>> mRiggingPalettesBuffer;
    graphics::UniformBufferObject mJointMatrices;};


} // namespace snacgame
} // namespace ad