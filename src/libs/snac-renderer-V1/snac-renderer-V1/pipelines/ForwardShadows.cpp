#include "ForwardShadows.h"

#include "../Cube.h"
#include "../ResourceLoad.h"

#include <math/Transformations.h>
#include <math/VectorUtilities.h>

#include <imguiui/Widgets.h>
#include <imguiui/Widgets-impl.h>


namespace ad {
namespace snac {


handy::StringId gPassSid{"pass"};
handy::StringId gDepthSid{"depth"};
handy::StringId gForwardShadowSid{"forward_shadow"};



ForwardShadows::ForwardShadows(const graphics::AppInterface & aAppInterface,
                               Load<Technique> & aTechniqueLoader) :
    mAppInterface{aAppInterface},
    screenQuad{
        .mStream = makeQuad(),
        .mMaterial = std::make_shared<Material>(Material{
            .mEffect = std::make_shared<Effect>(),
        }),
        .mName = "screen_quad",
    }
{
    graphics::allocateStorage(depthMap, GL_DEPTH_COMPONENT24, gShadowMapSize);
    {
        auto boundDepthMap = graphics::ScopedBind{depthMap};
        glTexParameteri(depthMap.mTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(depthMap.mTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(depthMap.mTarget, 
                         GL_TEXTURE_BORDER_COLOR,
                         math::hdr::Rgba_f{1.f, 0.f, 0.f, 0.f}.data());
    }

    graphics::attachImage(depthFBO, depthMap, GL_DEPTH_ATTACHMENT);

    screenQuad.mMaterial->mEffect->mTechniques.push_back(
        aTechniqueLoader.get("shaders/ShowDepth.prog"));
}


void ForwardShadows::Controls::drawGui()
{
    using namespace imguiui;

    ImGui::Begin("ForwardShadow");
    Guard scopeWindow([]()
    {
        ImGui::End();
    });

    GLfloat bias = mShadowBias;
    ImGui::DragFloat("Bias", &bias, 0.000001f, -0.01f, 0.01f, "%.6f");
    mShadowBias = bias;
    
    addCombo("Depth filtering", mDetphMapFilter, std::span{gAvailableFilters});
    addCombo("Culled faces", mCullFaceMode, std::span{gCullFaceModes});
    addCheckbox("Show depthmap", mShowDepthMap);

    ImGui::InputFloat2("Depthmap range", mDepthMapRange.data());
}


void ForwardShadows::execute(
    const std::vector<Pass::Visual> & aEntities,
    const Camera & aLightViewpoint,
    Renderer & aRenderer,
    ProgramSetup & aProgramSetup)
{
    auto scopeDepth = graphics::scopeFeature(GL_DEPTH_TEST, true);

    auto scopeCullFeature = graphics::scopeFeature(GL_CULL_FACE, true);

    auto scopeUniforms = aProgramSetup.mUniforms.push({
            // Note: Distance means "absolute value" here, so negate the plane Z coordinate (which is negative)
            {snac::Semantic::NearDistance, -aLightViewpoint.getCurrentParameters().zNear},
            {snac::Semantic::FarDistance,  -aLightViewpoint.getCurrentParameters().zFar},
    });

    // GL_LINEAR seems required to get hardware PCF with sampler2DShadow.
    graphics::setFiltering(depthMap, mControls.mDetphMapFilter);

    // Render shadow map
    {
        auto scopeCullMode = graphics::scopeCullFace(mControls.mCullFaceMode);

        static const Pass depthMapPass{"Depth-map", {{gPassSid, gDepthSid}}};

        graphics::ScopedBind fboScope{depthFBO};

        glViewport(0, 0, gShadowMapSize.width(), gShadowMapSize.height());
        glClear(GL_DEPTH_BUFFER_BIT);

        auto scopePush = 
            aProgramSetup.mUniforms.push(Semantic::ViewingMatrix, aLightViewpoint.assembleViewMatrix());

        depthMapPass.draw(aEntities, aRenderer, aProgramSetup);
    }

    // Default framebuffer rendering
    clear();

    // From the spec core 4.6, p326
    // "If a texel has been written, then in order to safely read the result a texel fetch
    // must be in a subsequent draw call separated by the command "
    // If find this odd, as shadow maps implementations online do not seem to use it.
    #if defined(GL_VERSION_4_5)
    glTextureBarrier();
    #endif

    // Render scene with shadows
    {
        static const Pass forwardShadowPass{"Forward-with-shadows", {{gPassSid, gForwardShadowSid}}};

        {
            graphics::ScopedBind scopedDepthMap{depthMap};
            glTexParameteri(depthMap.mTarget, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(depthMap.mTarget, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        }

        glViewport(0,
                   0, 
                   mAppInterface.getFramebufferSize().width(), 
                   mAppInterface.getFramebufferSize().height());

        auto texturePush = aProgramSetup.mTextures.push(Semantic::ShadowMap, &depthMap);

        auto uniformPush = 
            aProgramSetup.mUniforms.push({
                {Semantic::LightViewingMatrix, aLightViewpoint.assembleViewMatrix()},
                {Semantic::ShadowBias, mControls.mShadowBias.load()},
            });

        forwardShadowPass.draw(aEntities, aRenderer, aProgramSetup);
    }

    // Show shadow map in a viewport
    if(mControls.mShowDepthMap)
    {
        static const Pass showDepthmapPass{"debug-show-depthmap"};

        {
            graphics::ScopedBind scopedDepthMap{depthMap};
            glTexParameteri(depthMap.mTarget, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        }

        GLsizei viewportHeight = mAppInterface.getFramebufferSize().height() / 4;
        Guard scopedViewport = graphics::scopeViewport({
            {0, 0}, 
            {
                (GLsizei)(viewportHeight * getRatio<GLfloat>(gShadowMapSize)), 
                   viewportHeight
            }
        });

        auto scopedTexture = aProgramSetup.mTextures.push(Semantic::BaseColorTexture, &depthMap);

        auto scopeUniforms = aProgramSetup.mUniforms.push(Semantic::RenormalizationRange,
                                                          mControls.mDepthMapRange);

        showDepthmapPass.draw(screenQuad, aRenderer, aProgramSetup);
    }
}


} // namespace snac
} // namespace ad
