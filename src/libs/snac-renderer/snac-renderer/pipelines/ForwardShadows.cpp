#include "ForwardShadows.h"

#include "../Cube.h"
#include "../ResourceLoad.h"

#include <math/Transformations.h>
#include <math/VectorUtilities.h>



namespace ad {
namespace snac {


handy::StringId gPassSid{"pass"};
handy::StringId gDepthSid{"depth"};
handy::StringId gForwardShadowSid{"forward_shadow"};


void addCheckbox(const char * aLabel, MovableAtomic<bool> & aValue)
{
    bool value = aValue;
    ImGui::Checkbox(aLabel, &value);
    aValue = value;
}


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
        //loadTechnique(mFinder.pathFor("shaders/ShowDepth.prog")));
}


void ForwardShadows::Controls::drawGui()
{
    ImGui::Begin("ForwardShadow");

    GLfloat bias = mShadowBias;
    ImGui::DragFloat("Bias", &bias, 0.000001f, 0.0f, 0.01f, "%.6f");
    mShadowBias = bias;
    
    static unsigned int item_current_idx = 0; // Here we store our selection data as an index.
    static ImGuiComboFlags flags = 0;
    // Pass in the preview value visible before opening the combo (it could be anything)
    const std::string combo_preview_value = graphics::to_string(mDetphMapFilter);
    if (ImGui::BeginCombo("Depth filtering", combo_preview_value.c_str(), flags))
    {
        for (unsigned int n = 0; n < std::size(gAvailableFilters); n++)
        {
            GLenum iteratedFilter = gAvailableFilters[n];
            const bool is_selected = (mDetphMapFilter == iteratedFilter);
            if (ImGui::Selectable(graphics::to_string(iteratedFilter).c_str(), is_selected))
            {
                mDetphMapFilter = iteratedFilter;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    addCheckbox("Show depthmap", mShowDepthMap);

    ImGui::End();
}


void ForwardShadows::execute(
    const std::vector<Pass::Visual> & aEntities,
    const Camera & aLightViewpoint,
    Renderer & aRenderer,
    ProgramSetup & aProgramSetup)
{
    auto scopeDepth = graphics::scopeFeature(GL_DEPTH_TEST, true);

    auto scopeUniforms = aProgramSetup.mUniforms.push({
            // Note: Distance means "absolute value" here, so negate the plane Z coordinate (which is negative)
            {snac::Semantic::NearDistance, -aLightViewpoint.getCurrentParameters().zNear},
            {snac::Semantic::FarDistance,  -aLightViewpoint.getCurrentParameters().zFar},
    });

    // GL_LINEAR seems required to get hardware PCF with sampler2DShadow.
    graphics::setFiltering(depthMap, mControls.mDetphMapFilter);

    // Render shadow map
    {
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

        showDepthmapPass.draw(screenQuad, aRenderer, aProgramSetup);
    }
}


} // namespace snac
} // namespace ad
