#include "ModelLoader.h"

#include <snacman/Profiling.h>
#include <snacman/Resources.h>

#include <snacman/simulations/snacgame/GameParameters.h>
#include <snacman/simulations/snacgame/GraphicState.h>
#include <snacman/simulations/snacgame/ImguiInhibiter.h>


namespace ad {
struct RawInput;

namespace snacgame {


ModelLoader::ModelLoader(graphics::AppInterface & aAppInterface,
                         snac::RenderThread<Renderer_t> & aRenderThread,
                         imguiui::ImguiUi & aImguiUi,
                         resource::ResourceFinder aResourceFinder,
                         arte::Freetype & aFreetype,
                         RawInput & aInput) :
    mAppInterface{&aAppInterface},
    mImguiUi{aImguiUi},
    mOrbitalControl{
        .mOrbital = {
            gInitialCameraSpherical.radius(),
            gInitialCameraSpherical.polar(),
            gInitialCameraSpherical.azimuthal()
        },
    }
{
    snac::Resources resources{
        std::move(aResourceFinder),
        aFreetype,
        aRenderThread
    };

    mModel ={
        .mIndex = 0,
        .mNode = resources.getModel("models/stage/stage.seum", "effects/MeshTextures.sefx"),
    };
}

void ModelLoader::drawDebugUi(snac::ConfigurableSettings & aSettings,
                              ImguiInhibiter & aInhibiter,
                              RawInput & aInput)
{
    TIME_RECURRING_FUNC(Main);

    {
        std::lock_guard lock{mImguiUi.mFrameMutex};

        mImguiUi.newFrame();
        Guard renderGuard{[this]() { this->mImguiUi.render(); }};

        aInhibiter.resetCapture(static_cast<ImguiInhibiter::WantCapture>(
            (mImguiUi.isCapturingMouse() ? ImguiInhibiter::Mouse
                                         : ImguiInhibiter::Null)
            | (mImguiUi.isCapturingKeyboard() ? ImguiInhibiter::Keyboard
                                              : ImguiInhibiter::Null)));
        //ImGui::Begin("Silly window");
        //ImGui::End();
    }
}

bool ModelLoader::update(snac::Clock::duration & aUpdatePeriod, RawInput & aInput)
{
    snac::DebugDrawer::StartFrame();

    mOrbitalControl.update(
        aInput,
        snac::Camera::gDefaults.vFov, // TODO Should be dynamic
        mAppInterface->getWindowSize().height());


    return aInput.mKeyboard.mKeyState[GLFW_KEY_ESCAPE];
}

std::unique_ptr<visu::GraphicState> ModelLoader::makeGraphicState()
{
    TIME_RECURRING_FUNC(Main);

    auto state = std::make_unique<visu::GraphicState>();

    //ent::Phase nomutation;

    ////
    //// Worldspace models
    ////
    //mQueryRenderable.get(nomutation)
    //    .each(
    //        [&state](ent::Handle<ent::Entity> aHandle,
    //                 const component::GlobalPose & aGlobPose,
    //                 // TODO #anim restore this constness (for the moment,
    //                 // animation mutate the rig's scene)
    //                 /*const*/ component::VisualModel & aVisualModel)
    //        {
    //    // Note: This feels bad to test component presence here
    //    // but we do not want to handle VisualModel the same way depending on
    //    // the presence of RigAnimation (and we do not have "negation" on
    //    // Queries, to separately get VisualModels without RigAnimation)
    //    visu::Entity::SkeletalAnimation skeletal;
    //    if (auto entity = *aHandle.get(); entity.has<component::RigAnimation>())
    //    {
    //        // TODO #RV2 animation
    //        //const auto & rigAnimation =
    //        //    aHandle.get()->get<component::RigAnimation>();
    //        //skeletal = visu::Entity::SkeletalAnimation{
    //        //    .mRig = &aVisualModel.mModel->mRig,
    //        //    .mAnimation = rigAnimation.mAnimation,
    //        //    .mParameterValue = rigAnimation.mParameterValue,
    //        //};
    //    }
        state->mEntities.insert(
            mModel.mIndex,
            visu::Entity{
                .mPosition_world = math::Position<3, GLfloat>{},
                .mScaling = {1.f, 1.f, 1.f},
                .mOrientation = math::Quaternion<GLfloat>::Identity(),
                .mColor = math::hdr::gWhite<GLfloat>,
                .mModel = mModel.mNode,
                //.mRigging = std::move(skeletal),
                //.mDisableInterpolation = aVisualModel.mDisableInterpolation,
            });
    //    // Note: Although it does not feel correct, this is a convenient place
    //    // to reset this flag
    //    aVisualModel.mDisableInterpolation = false;
    //    });

    ////
    //// Worldspace Text
    ////
    //mQueryTextWorld.get(nomutation)
    //    .each(
    //        [&state](ent::Handle<ent::Entity> aHandle, component::Text & aText,
    //                 component::GlobalPose & aGlobPose)
    //        {
    //    state->mTextWorldEntities.insert(
    //        aHandle.id(),
    //        visu::Text{
    //            .mPosition_world = aGlobPose.mPosition,
    //            // TODO remove the hardcoded value of 100
    //            // Note hardcoded 100 scale down. Because I'd like a value of 1
    //            // for the scale of the component to still mean "about visible".
    //            .mScaling =
    //                aGlobPose.mInstanceScaling * aGlobPose.mScaling / 100.f,
    //            .mOrientation = aGlobPose.mOrientation,
    //            .mString = aText.mString,
    //            .mFont = aText.mFont,
    //            .mColor = aText.mColor,
    //        });
    //    });

    ////
    //// Screenspace Text
    ////
    //mQueryTextScreen.get(nomutation)
    //    .each(
    //        [&state, this](ent::Handle<ent::Entity> aHandle,
    //                       component::Text & aText,
    //                       component::PoseScreenSpace & aPose)
    //        {
    //    math::Position<3, float> position_screenPix{
    //        aPose.mPosition_u.cwMul(
    //            // TODO this multiplication should be done once and cached
    //            // but it should be refreshed on framebuffer resizing.
    //            static_cast<math::Position<2, GLfloat>>(
    //                this->mAppInterface->getFramebufferSize())
    //            / 2.f),
    //        0.f};

    //    state->mTextScreenEntities.insert(
    //        aHandle.id(),
    //        visu::Text{
    //            .mPosition_world = position_screenPix,
    //            .mScaling = math::Size<3, float>{aPose.mScale, 1.f},
    //            .mOrientation =
    //                math::Quaternion{
    //                    math::UnitVec<3, float>::MakeFromUnitLength(
    //                        {0.f, 0.f, 1.f}),
    //                    aPose.mRotationCCW},
    //            .mString = aText.mString,
    //            .mFont = aText.mFont,
    //            .mColor = aText.mColor,
    //        });
    //    });

    state->mCamera = mOrbitalControl.getCameraState();

    //state->mDebugDrawList = snac::DebugDrawer::EndFrame();

    return state;
}

} // namespace snacgame
} // namespace ad
