#pragma once

#include "Gui.h"

#include <graphics/AppInterface.h>
#include <graphics/CameraUtilities.h>

#include <handy/StringId.h>

#include <resource/ResourceFinder.h>

#include <math/Transformations.h>

#include <snac-renderer-V1/Camera.h>
#include <snac-renderer-V1/Cube.h>
#include <snac-renderer-V1/ResourceLoad.h>
#include <snac-renderer-V1/pipelines/ForwardShadows.h> 


namespace ad {
namespace snac {


handy::StringId gViewSid{"view"};
handy::StringId gLeftSid{"left"};
handy::StringId gRightSid{"right"};

struct PoseColor
{
    math::Matrix<4, 4, float> pose;
    math::sdr::Rgba albedo{math::sdr::gWhite / 2};
};


// Make that dynamic too...
InstanceStream prepareInstances()
{
    InstanceStream instances;
    std::size_t instanceSize = sizeof(PoseColor);
    instances.mInstanceBuffer.mStride = (GLsizei)instanceSize;

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


InstanceStream populateTripleInstances(math::Box<GLfloat> aBoundingBox)
{
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

    InstanceStream instances = prepareInstances();
    instances.mInstanceCount = (GLsizei)std::size(transformations),

    bind(instances.mInstanceBuffer.mBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(PoseColor) * std::size(transformations),
        transformations.data(),
        GL_DYNAMIC_DRAW);

    return instances;
}

InstanceStream populateInstances(std::vector<PoseColor> aInstances)
{
    InstanceStream instances = prepareInstances();
    instances.mInstanceCount = (GLsizei)std::size(aInstances),

    bind(instances.mInstanceBuffer.mBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(PoseColor) * std::size(aInstances),
        aInstances.data(),
        GL_DYNAMIC_DRAW);

    return instances;
}

void duplicateTechniques(Effect & aEffect)
{
    std::vector<Technique> & techniques = aEffect.mTechniques;
    // Reallocation would crash the loop.
    techniques.reserve(techniques.size() * 2);
    for (auto & technique : techniques)
    {
        technique.mAnnotations.emplace(gViewSid, gLeftSid);
        techniques.push_back(Technique{
            .mAnnotations = {{gViewSid, gRightSid}},
            .mProgram = loadProgram(technique.mProgramFile),
            .mProgramFile = technique.mProgramFile,
        });
    }
}


template <class T_value, std::size_t N>
std::vector<T_value> moveInitVector(std::array<T_value, N> aValues)
{
    return std::vector<T_value>{
        std::make_move_iterator(aValues.begin()),
        std::make_move_iterator(aValues.end())
    };
}


struct Scene
{
    Scene(graphics::ApplicationGlfw & aGlfwApp,
          Model aModel,
          const resource::ResourceFinder & aFinder);

    void update();

    void recompileRightView();

    void render(Renderer & aRenderer);

    void drawSideBySide(Renderer & aRenderer,
                        ProgramSetup & aProgramSetup);

    std::vector<Pass::Visual> getVisuals() const;

    const graphics::AppInterface & mAppInterface;

    struct Entity
    {
        InstanceStream mInstances;
        Model mModel;
    };
    std::vector<Entity> mEntities;

    Camera mCamera;
    CameraBuffer mCameraBuffer;
    MouseOrbitalControl mCameraControl;
    const resource::ResourceFinder & mFinder;

    ForwardShadows mDrawerShadows;

    std::shared_ptr<graphics::AppInterface::SizeListener> mSizeListening;
    Gui mGui;
};


inline Scene::Scene(graphics::ApplicationGlfw & aGlfwApp,
                    Model aModel,
                    const resource::ResourceFinder & aFinder) :
    mAppInterface{*aGlfwApp.getAppInterface()},
    mEntities{ 
        moveInitVector<Entity, 1>({Entity{
            // Is it safe? Do we have a guarantee regarding member order?
            // (see the std::move)
            .mInstances = populateTripleInstances(aModel.mBoundingBox),
            .mModel = std::move(aModel),
        }})
    },
    mCamera{math::getRatio<float>(aGlfwApp.getAppInterface()->getFramebufferSize()), Camera::gDefaults},
    mCameraControl{aGlfwApp.getAppInterface()->getWindowSize(), Camera::gDefaults.vFov},
    mFinder{aFinder},
    mDrawerShadows{*aGlfwApp.getAppInterface(), TechniqueLoader{aFinder}},
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
            mCamera.setPerspectiveProjection(math::getRatio<float>(size), Camera::gDefaults); 
            mCameraControl.setWindowSize(size);
        });

    // Add floor
    {
        Mesh floor{
            .mStream = makeRectangle(math::Rectangle<GLfloat>{{-5.f, -5.f}, {10.f, 10.f}}),
            .mMaterial = std::make_shared<Material>(Material{
                .mEffect = loadEffect(mFinder.pathFor("effects/Mesh.sefx"), TechniqueLoader{mFinder}),
            }),
            .mName = "floor",
        };
        mEntities.push_back({
            .mInstances = populateInstances({{
                PoseColor{
                    .pose = math::trans3d::rotateX(math::Degree<float>(-90.f)) // Normals to face "up"
                        * math::trans3d::translate(math::Vec<3, float>{ 0.f, -2.f, 0.f}),
                }
            }}),
            .mModel = {
                .mParts = moveInitVector<Mesh, 1>({std::move(floor),}),
            }
        });
    }


    // Annotate the existing techniques as "view: left"
    // Add a copy of each technique, annotated "view: right"
    std::set<Effect*> duplicatedEffects;
    for(auto & [_instances, model] : mEntities)
    {
        for(auto & mesh : model.mParts)
        {
            // Only duplicate the techniques if this effect was not already present.
            if(duplicatedEffects.insert(mesh.mMaterial->mEffect.get()).second)
            {
                duplicateTechniques(*mesh.mMaterial->mEffect);
            }
        }
    }
}


std::vector<Pass::Visual> Scene::getVisuals() const
{
    std::vector<Pass::Visual> visuals;
    visuals.reserve(mEntities.size());
    for (const auto & [instance, model] : mEntities)
    {
        for (const auto & mesh : model.mParts)
        {
            visuals.push_back({&mesh, &instance});
        }
    }
    return visuals;
}


inline void Scene::update()
{
    mCamera.setPose(mCameraControl.getParentToLocal());
    mCameraBuffer.set(mCamera);
}


inline void Scene::recompileRightView()
{
    for(auto & [_instances, model] : mEntities)
    {
        for (const auto & mesh : model.mParts)
        {
            for (auto & technique : mesh.mMaterial->mEffect->mTechniques)
            {
                if(technique.mAnnotations.at(gViewSid) == gRightSid)
                {
                    attemptRecompile(technique);
                }
            }
        }
    }
}


inline void Scene::render(Renderer & aRenderer)
{
    math::hdr::Rgb_f lightColor =  to_hdr<float>(math::sdr::gWhite) * 0.8f;
    math::Position<3, GLfloat> lightPosition{0.f, 0.f, 0.f};
    math::hdr::Rgb_f ambientColor =  math::hdr::Rgb_f{0.1f, 0.1f, 0.1f};

    ProgramSetup setup{
        .mUniforms{
            {snac::Semantic::LightColor, snac::UniformParameter{lightColor}},
            // TODO Consolidate the light used in forward phong shading (currently the camera)
            // with the light used to generate shadow (currently fixed above the scene)
            {snac::Semantic::LightPosition, {lightPosition}},
            {snac::Semantic::AmbientColor, {ambientColor}},
        },
        .mUniformBlocks{
            {BlockSemantic::Viewing, &mCameraBuffer.mViewing},
        },
    };

    // Note: assumes a square shadow map ATM
    Camera shadowLightViewPoint{1, Camera::gDefaults};
    shadowLightViewPoint.setPose(
        math::trans3d::rotateX(math::Degree<float>{55.f})
        * math::trans3d::translate<GLfloat>({0.f, -1.f, -15.f}));

    {
        auto scopedUI = mGui.startUI();
        ImGui::Begin("Render Controls");

        static const char* modes[] = {"Side-by-side", "Shadows"};
        static int currentMode = 0;
        ImGui::ListBox("Mode", &currentMode, modes, IM_ARRAYSIZE(modes), 4);
        switch(currentMode)
        {
            case 0:
                drawSideBySide(aRenderer, setup);
                break;
            case 1:
                mDrawerShadows.execute(getVisuals(), shadowLightViewPoint, aRenderer, setup);
                mDrawerShadows.drawGui();
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
                           ProgramSetup & aProgramSetup)
{
    clear();

    auto visuals = getVisuals();

    auto scopeDepth = graphics::scopeFeature(GL_DEPTH_TEST, true);

    auto scissorScope = graphics::scopeFeature(GL_SCISSOR_TEST, true);

    glViewport(0,
               0, 
               mAppInterface.getFramebufferSize().width(), 
               mAppInterface.getFramebufferSize().height());

    // Left view
    static const Pass leftViewPass{"left-view", {{gViewSid, gLeftSid}}};

    glScissor(0,
              0,
              mAppInterface.getFramebufferSize().width()/2,
              mAppInterface.getFramebufferSize().height());

    leftViewPass.draw(visuals, aRenderer, aProgramSetup);

    // Right view
    static const Pass rightViewPass{"right-view", {{gViewSid, gRightSid}}};

    glScissor(mAppInterface.getFramebufferSize().width()/2,
              0, 
              mAppInterface.getFramebufferSize().width(),
              mAppInterface.getFramebufferSize().height());

    rightViewPass.draw(visuals, aRenderer, aProgramSetup);
}


} // namespace snac
} // namespace ad