#include "DrawerShadows.h"

#include <math/Transformations.h>
#include <math/VectorUtilities.h>

#include <snac-renderer/ResourceLoad.h>


namespace ad {
namespace snac {


handy::StringId gPassSid{"pass"};
handy::StringId gDepthSid{"depth"};
handy::StringId gForwardShadowSid{"forward_shadow"};


void DrawerShadows::drawGui()
{
    ImGui::Begin("Shadow Controls");

    ImGui::DragFloat("Bias", &mShadowBias, 0.000001f, 0.0f, 0.01f, "%.6f");
    
    static const std::array<GLenum, 2> filtering{GL_NEAREST, GL_LINEAR};
    static int item_current_idx = 0; // Here we store our selection data as an index.
    static ImGuiComboFlags flags = 0;
    const std::string combo_preview_value = graphics::to_string(filtering[item_current_idx]);  // Pass in the preview value visible before opening the combo (it could be anything)
    if (ImGui::BeginCombo("Depth filtering", combo_preview_value.c_str(), flags))
    {
        for (int n = 0; n < std::size(filtering); n++)
        {
            const bool is_selected = (item_current_idx == n);
            if (ImGui::Selectable(graphics::to_string(filtering[n]).c_str(), is_selected))
            {
                item_current_idx = n;
                mDetphMapFilter = filtering[n];
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::End();
}


void DrawerShadows::draw(
    const VisualEntities & aEntities,
    Renderer & aRenderer,
    const UniformRepository & aUniforms,
    const snac::UniformBlocks & aUniformBlocks)
{
    drawGui();

    graphics::Texture depthMap{GL_TEXTURE_2D};
    graphics::allocateStorage(depthMap, GL_DEPTH_COMPONENT24, gShadowMapSize);
    // GL_LINEAR seems required to get hardware PCF with sampler2DShadow.
    graphics::setFiltering(depthMap, mDetphMapFilter);
    {
        graphics::ScopedBind scopedDepthMap{depthMap};
        glTexParameteri(depthMap.mTarget, GL_TEXTURE_COMPARE_MODE , GL_NONE);
    }

    graphics::FrameBuffer depthFBO;
    graphics::attachImage(depthFBO, depthMap, GL_DEPTH_ATTACHMENT);

    Camera lightViewPoint{math::getRatio<GLfloat>(gShadowMapSize), Camera::gDefaults};
    lightViewPoint.setPose(
        math::trans3d::rotateX(math::Degree{55.f})
        * math::trans3d::translate<GLfloat>({0.f, -1.f, -15.f}));

    // Render shadow map
    {
        graphics::ScopedBind fboScope{depthFBO};

        glViewport(0, 0, gShadowMapSize.width(), gShadowMapSize.height());
        glClear(GL_DEPTH_BUFFER_BIT);

        UniformRepository uniforms{aUniforms};
        uniforms.emplace(Semantic::ViewingMatrix, lightViewPoint.assembleViewMatrix());

        renderEntities(aEntities,
                       aRenderer,
                       uniforms, 
                       aUniformBlocks,
                       {},
                       { {gPassSid, gDepthSid}, });
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
        {
            graphics::ScopedBind scopedDepthMap{depthMap};
            glTexParameteri(depthMap.mTarget, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(depthMap.mTarget, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        }

        glViewport(0,
                   0, 
                   mAppInterface.getFramebufferSize().width(), 
                   mAppInterface.getFramebufferSize().height());

        TextureRepository textures{ 
            {Semantic::ShadowMap, &depthMap},
        };

        UniformRepository uniforms{aUniforms};
        uniforms.emplace(Semantic::LightViewingMatrix, lightViewPoint.assembleViewMatrix());
        uniforms.emplace(Semantic::ShadowBias, mShadowBias);

        renderEntities(aEntities,
                       aRenderer,
                       uniforms, 
                       aUniformBlocks,
                       textures,
                       { {gPassSid, gForwardShadowSid}, });
    }

    // Show shadow map in a viewport
    {
        {
            graphics::ScopedBind scopedDepthMap{depthMap};
            glTexParameteri(depthMap.mTarget, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        }

        Mesh screenQuad{
            .mStream = makeQuad(),
            .mMaterial = std::make_shared<Material>(Material{
                .mEffect = std::make_shared<Effect>(),
            }),
            .mName = "screen_quad",
        };

        screenQuad.mMaterial->mTextures.emplace(Semantic::BaseColorTexture, &depthMap);
        screenQuad.mMaterial->mEffect->mTechniques.push_back(
            loadTechnique(mFinder.pathFor("shaders/ShowDepth.prog")));

        GLsizei viewportHeight = mAppInterface.getFramebufferSize().height() / 4;
        glViewport(0,
                0, 
                (GLsizei)(viewportHeight * getRatio<GLfloat>(gShadowMapSize)), 
                viewportHeight);

        aRenderer.render(screenQuad,
                         gNotInstanced,
                         aUniforms,
                         aUniformBlocks);
    }
}


} // namespace snac
} // namespace ad
