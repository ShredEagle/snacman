#pragma once

#include "Gui.h"

#include <graphics/AppInterface.h>

#include <handy/StringId.h>

#include <resource/ResourceFinder.h>

#include <math/Transformations.h>
#include <math/VectorUtilities.h>

#include <snac-renderer/Camera.h>
#include <snac-renderer/Cube.h>
#include <snac-renderer/Render.h>
#include <snac-renderer/ResourceLoad.h>

#include <renderer/FrameBuffer.h>
#include <renderer/ScopeGuards.h>


namespace ad {
namespace snac {


handy::StringId gViewSid{"view"};
handy::StringId gLeftSid{"left"};
handy::StringId gRightSid{"right"};

handy::StringId gPassSid{"pass"};
handy::StringId gDepthSid{"depth"};


InstanceStream makeInstances(math::Box<GLfloat> aBoundingBox)
{
    
    struct PoseColor
    {
        math::Matrix<4, 4, float> pose;
        math::sdr::Rgba albedo{math::sdr::gWhite / 2};
    };

    GLfloat maxMagnitude = *aBoundingBox.dimension().getMaxMagnitudeElement();
    if(maxMagnitude == 0.)
    {
        maxMagnitude =  1;
        SELOG(warn)("Model bounding box is null, assuming unit size.");
    }

    auto scaling = math::trans3d::scaleUniform(3.f / maxMagnitude);

    math::Vec<3, float> centerOffset = aBoundingBox.center().as<math::Vec>() * scaling;

    std::vector<PoseColor> transformations{
        {scaling * math::trans3d::translate(math::Vec<3, float>{ 0.f, 0.f, -1.f} - centerOffset)},
        {scaling * math::trans3d::translate(math::Vec<3, float>{-4.f, 0.f, -1.f} - centerOffset)},
        {scaling * math::trans3d::translate(math::Vec<3, float>{ 4.f, 0.f, -1.f} - centerOffset)},
    };

    InstanceStream instances{
        .mInstanceCount = (GLsizei)std::size(transformations),
    };
    std::size_t instanceSize = sizeof(decltype(transformations)::value_type);
    instances.mInstanceBuffer.mStride = (GLsizei)instanceSize;

    bind(instances.mInstanceBuffer.mBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        instanceSize * std::size(transformations),
        transformations.data(),
        GL_DYNAMIC_DRAW);

    {
        graphics::ClientAttribute transformation{
            .mDimension = {4, 4},
            .mOffset = offsetof(PoseColor, pose),
            .mComponentType = GL_FLOAT,
        };
        instances.mAttributes.emplace(Semantic::LocalToWorld, transformation);
    }
    {
        graphics::ClientAttribute albedo{
            .mDimension = 4,
            .mOffset = offsetof(PoseColor, albedo),
            .mComponentType = GL_UNSIGNED_BYTE,
        };
        instances.mAttributes.emplace(Semantic::Albedo, albedo);
    }
    return instances;
}


struct Scene
{
    Scene(graphics::ApplicationGlfw & aGlfwApp,
          Mesh aMesh,
          const resource::ResourceFinder & aFinder);

    void update();

    void recompileRightView();

    void render(Renderer & aRenderer);

    void drawSideBySide(Renderer & aRenderer,
                        const snac::UniformRepository & aUniforms,
                        const snac::UniformBlocks & aUniformBlocks);

    void drawShadows(Renderer & aRenderer,
                     const snac::UniformRepository & aUniforms,
                     const snac::UniformBlocks & aUniformBlocks);

    const graphics::AppInterface & mAppInterface;

    Mesh mMesh;
    InstanceStream mInstances{makeInstances(mMesh.mStream.mBoundingBox)};
    Camera mCamera;
    MouseOrbitalControl mCameraControl;
    const resource::ResourceFinder & mFinder;

    std::shared_ptr<graphics::AppInterface::SizeListener> mSizeListening;
    Gui mGui;
};


inline Scene::Scene(graphics::ApplicationGlfw & aGlfwApp,
                    Mesh aMesh,
                    const resource::ResourceFinder & aFinder) :
    mAppInterface{*aGlfwApp.getAppInterface()},
    mMesh{std::move(aMesh)},
    mCamera{math::getRatio<float>(aGlfwApp.getAppInterface()->getFramebufferSize()), Camera::gDefaults},
    mCameraControl{aGlfwApp.getAppInterface()->getWindowSize(), Camera::gDefaults.vFov},
    mFinder{aFinder},
    mGui{aGlfwApp}
{
    graphics::AppInterface & appInterface = *aGlfwApp.getAppInterface();

    appInterface.registerCursorPositionCallback(
        [this](double xpos, double ypos)
        {
            if(!mGui.mImguiUi.isCapturingMouse())
            {
                mCameraControl.callbackCursorPosition(xpos, ypos);
            }
        });

    appInterface.registerMouseButtonCallback(
        [this](int button, int action, int mods, double xpos, double ypos)
        {
            if(!mGui.mImguiUi.isCapturingMouse())
            {
                mCameraControl.callbackMouseButton(button, action, mods, xpos, ypos);
            }
        });

    appInterface.registerScrollCallback(
        [this](double xoffset, double yoffset)
        {
            if(!mGui.mImguiUi.isCapturingMouse())
            {
                mCameraControl.callbackScroll(xoffset, yoffset);
            }
        });

    mSizeListening = appInterface.listenWindowResize(
        [this](const math::Size<2, int> & size)
        {
            mCamera.resetProjection(math::getRatio<float>(size), Camera::gDefaults); 
            mCameraControl.setWindowSize(size);
        });

    // Annotate the existing techniques as "view: left"
    // Add a copy of each technique, annotated "view: right"
    std::vector<Technique> & techniques = mMesh.mMaterial->mEffect->mTechniques;
    // Reallocation would crash the loop.
    techniques.reserve(techniques.size() * 2);
    for (auto & technique : mMesh.mMaterial->mEffect->mTechniques)
    {
        technique.mAnnotations.emplace(gViewSid, gLeftSid);
        mMesh.mMaterial->mEffect->mTechniques.push_back(Technique{
            .mAnnotations = {{gViewSid, gRightSid}},
            .mProgram = loadProgram(technique.mProgramFile),
            .mProgramFile = technique.mProgramFile,
        });
    }


}


inline void Scene::update()
{
    mCamera.setWorldToCamera(mCameraControl.getParentToLocal());
}


inline void Scene::recompileRightView()
{
    for (auto & technique : mMesh.mMaterial->mEffect->mTechniques)
    {
        if(technique.mAnnotations.at(gViewSid) == gRightSid)
        {
            attemptRecompile(technique);
        }
    }
}


inline void Scene::render(Renderer & aRenderer)
{
    UniformBlocks uniformBlocks{
         {BlockSemantic::Viewing, &mCamera.mViewing},
    };

    math::hdr::Rgb_f lightColor =  to_hdr<float>(math::sdr::gWhite) * 0.8f;
    math::Position<3, GLfloat> lightPosition{0.f, 0.f, 0.f};
    math::hdr::Rgb_f ambientColor =  math::hdr::Rgb_f{0.1f, 0.2f, 0.1f};

    const snac::UniformRepository uniforms{
        {snac::Semantic::LightColor, snac::UniformParameter{lightColor}},
        {snac::Semantic::LightPosition, {lightPosition}},
        {snac::Semantic::AmbientColor, {ambientColor}},
        {snac::Semantic::NearDistance, -mCamera.getCurrentParameters().zNear},
        {snac::Semantic::FarDistance,  -mCamera.getCurrentParameters().zFar},
    };

    {
        auto scopedUI = mGui.startUI();
        ImGui::Begin("Render Controls");

        static const char* modes[] = {"Side-by-side", "Shadows"};
        static int currentMode = 0;
        ImGui::ListBox("Mode", &currentMode, modes, IM_ARRAYSIZE(modes), 4);
        switch(currentMode)
        {
            case 0:
                drawSideBySide(aRenderer, uniforms, uniformBlocks);
                break;
            case 1:
                drawShadows(aRenderer, uniforms, uniformBlocks);
                break;
        }

        if(ImGui::Button("Recompile Right"))
        {
            recompileRightView();
        }

        ImGui::End();
    }

    mGui.render();
}


void Scene::drawSideBySide(Renderer & aRenderer,
                           const snac::UniformRepository & aUniforms,
                           const snac::UniformBlocks & aUniformBlocks)
{
    clear();

    auto scissorScope = graphics::scopeFeature(GL_SCISSOR_TEST, true);

    glViewport(0,
               0, 
               mAppInterface.getFramebufferSize().width(), 
               mAppInterface.getFramebufferSize().height());

    // Left view
    glScissor(0,
                0,
                mAppInterface.getFramebufferSize().width()/2,
                mAppInterface.getFramebufferSize().height());

    aRenderer.render(mMesh,
                     mInstances,
                     aUniforms,
                     aUniformBlocks,
                     { {gViewSid, gLeftSid}, });
    // Right view
    glScissor(mAppInterface.getFramebufferSize().width()/2,
                0, 
                mAppInterface.getFramebufferSize().width(),
                mAppInterface.getFramebufferSize().height());

    aRenderer.render(mMesh,
                     mInstances,
                     aUniforms,
                     aUniformBlocks,
                     { {gViewSid, gRightSid}, });
}


inline void Scene::drawShadows(
    Renderer & aRenderer,
    const UniformRepository & aUniforms,
    const snac::UniformBlocks & aUniformBlocks)
{
    constexpr math::Size<2, int> shadowMapSize{1024, 1024};
    graphics::Texture depthMap{GL_TEXTURE_2D};
    graphics::allocateStorage(depthMap, GL_DEPTH_COMPONENT24, shadowMapSize);
    graphics::setFiltering(depthMap, GL_NEAREST);
    {
        graphics::ScopedBind scopedDepthMap{depthMap};
        glTexParameteri(depthMap.mTarget, GL_TEXTURE_COMPARE_MODE , GL_NONE);
    }

    graphics::FrameBuffer depthFBO;
    graphics::attachImage(depthFBO, depthMap, GL_DEPTH_ATTACHMENT);

    {
        graphics::ScopedBind fboScope{depthFBO};

        glViewport(0, 0, shadowMapSize.width(), shadowMapSize.height());
        glClear(GL_DEPTH_BUFFER_BIT);

        aRenderer.render(mMesh,
                         mInstances,
                         aUniforms,
                         aUniformBlocks,
                         { {gPassSid, gDepthSid}, });
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

    glViewport(0,
               0, 
               mAppInterface.getFramebufferSize().width(), 
               mAppInterface.getFramebufferSize().height());

    clear();

    aRenderer.render(screenQuad,
                     gNotInstanced,
                     aUniforms,
                     aUniformBlocks);
}


} // namespace snac
} // namespace ad
