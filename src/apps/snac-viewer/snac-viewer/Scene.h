#pragma once

#include "Gui.h"

#include <graphics/AppInterface.h>

#include <handy/StringId.h>

#include <math/Transformations.h>
#include <math/VectorUtilities.h>

#include <snac-renderer/Camera.h>
#include <snac-renderer/Cube.h>
#include <snac-renderer/Render.h>
#include <snac-renderer/ResourceLoad.h>


namespace ad {
namespace snac {

handy::StringId gViewSid{"view"};
handy::StringId gLeftSid{"left"};
handy::StringId gRightSid{"right"};

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
    Scene(graphics::ApplicationGlfw & aGlfwApp, Mesh aMesh);

    void update();

    void drawUI();

    void recompileRightView();

    void render(Renderer & aRenderer);

    const graphics::AppInterface & mAppInterface;

    Mesh mMesh;
    InstanceStream mInstances{makeInstances(mMesh.mStream.mBoundingBox)};
    Camera mCamera;
    MouseOrbitalControl mCameraControl;

    std::shared_ptr<graphics::AppInterface::SizeListener> mSizeListener;
    Gui mGui;
};


inline Scene::Scene(graphics::ApplicationGlfw & aGlfwApp, Mesh aMesh) :
    mAppInterface{*aGlfwApp.getAppInterface()},
    mMesh{std::move(aMesh)},
    mCamera{math::getRatio<float>(aGlfwApp.getAppInterface()->getFramebufferSize()), Camera::gDefaults},
    mCameraControl{aGlfwApp.getAppInterface()->getWindowSize(), Camera::gDefaults.vFov},
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

    mSizeListener = appInterface.listenWindowResize(
        [this](const math::Size<2, int> & size)
        {
            mCamera.resetProjection(math::getRatio<float>(size), Camera::gDefaults); 
            mCameraControl.setWindowSize(size);
        });

    // Annotate the existing techniques as view: left
    // Add a copy of each technique, annotated view: right
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


inline void Scene::drawUI()
{
    auto scopedUI = mGui.startUI();
    ImGui::Begin("Render Controls");
    if(ImGui::Button("Recompile Right"))
    {
        recompileRightView();
    }
    ImGui::End();
}


inline void Scene::recompileRightView()
{
    for (auto & technique : mMesh.mMaterial->mEffect->mTechniques)
    {
        if(technique.mAnnotations.at(gViewSid) == gRightSid)
        {
            try 
            {
                technique.mProgram = loadProgram(technique.mProgramFile);
            }
            catch(graphics::ShaderCompilationError & aError)
            {
                SELOG(error)
                    ("Cannot recompile program from file '{}' due to shader compilation error:\n{}",
                    technique.mProgramFile.string(), aError.what());
            }
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

    snac::UniformRepository uniforms{
        {snac::Semantic::LightColor, snac::UniformParameter{lightColor}},
        {snac::Semantic::LightPosition, {lightPosition}},
        {snac::Semantic::AmbientColor, {ambientColor}},
    };

    clear();

    // TODO There is something fishy here:
    // If this call is moved after the scene rendering, there are GL errors.
    drawUI();

    {
        auto scissorScope = graphics::scopeFeature(GL_SCISSOR_TEST, true);

        // Left view
        glScissor(0,
                  0,
                  mAppInterface.getFramebufferSize().width()/2,
                  mAppInterface.getFramebufferSize().height());

        aRenderer.render(mMesh,
                         mInstances,
                         uniforms,
                         uniformBlocks,
                         { {gViewSid, gLeftSid}, });
        // Right view
        glScissor(mAppInterface.getFramebufferSize().width()/2,
                  0, 
                  mAppInterface.getFramebufferSize().width(),
                  mAppInterface.getFramebufferSize().height());

        aRenderer.render(mMesh,
                         mInstances,
                         uniforms,
                         uniformBlocks,
                         { {gViewSid, gRightSid}, });
    }

    mGui.render();
}


} // namespace snac
} // namespace ad
