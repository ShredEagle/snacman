#include "SplashScene.h"

#include "DataScene.h"
#include "MenuScene.h"

#include "../GameContext.h"
#include "../typedef.h"

#include "../component/Text.h"
#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/GlobalPose.h"
#include "../component/VisualModel.h"

#include "../system/SceneStack.h"

#include <snacman/Profiling.h>

#include <snacman/simulations/snacgame/Entities.h>
#include <snacman/simulations/snacgame/InputCommandConverter.h>

#include <snacman/simulations/snacgame/system/InputProcessor.h>
#include <snacman/simulations/snacgame/system/SceneGraphResolver.h>

#include <handy/StringUtilities.h>

#include <math/Interpolation/ParameterAnimation.h>

#include <snac-renderer-V2/Model.h>


namespace ad {
namespace snacgame {
namespace scene {


namespace {

    ent::Handle<ent::Entity> makeEntity(GameContext & aContext)
    {
        auto handle = aContext.mWorld.addEntity("fullquad_entity");
        ent::Phase phase;
        handle.get(phase)
            ->add(component::GlobalPose{
                .mScaling = 0.75f,
                .mInstanceScaling = {
                    1.f / getRatio<float>(aContext.mAppInterface.getFramebufferSize()), 1.f, 1.f},
                .mColor = {1.f, 1.f, 1.f, 0.f},
            })
            .add(component::VisualModel{
                .mModel = aContext.mResources.getModel("models/splash/splash.seum", "effects/Splash.sefx"),
            });

        return handle;
    }

} // unnamed namespace

SplashScene::SplashScene(GameContext & aGameContext,
                         ent::Wrap<component::MappingContext> & aContext,
                         renderer::Environment & aEnvMap) :
    Scene(gSplashSceneName, aGameContext, aContext),
    mEnvMapReference{aEnvMap},
    mEnvMapToRestore{mEnvMapReference}
{
    mSplashEntity = makeEntity(aGameContext);
    mOwnedEntities.push_back(mSplashEntity);

    // TODO get rid of this Hack: 
    // its disables envmap by replacing it with default cted
    // it is later restored onExit()
    mEnvMapReference = {};
}

void SplashScene::onEnter(Transition) {
    mSystems = mGameContext.mWorld.addEntity("System for Splash");
    ent::Phase onEnter;
    mSystems.get(onEnter)
        ->add(system::SceneGraphResolver{mGameContext, mGameContext.mSceneRoot})
    ;

    mTimeAccu = snac::Clock::duration{0};
}

void SplashScene::onExit(Transition)
{
    // There is 1 transition possible
    // To the main menu
    ent::Phase destroy;
    clearEntities(destroy);
    mSystems.get(destroy)->erase();

    mEnvMapReference = mEnvMapToRestore;
}


void SplashScene::clearEntities(ent::Phase & aPhase)
{
    for (auto handle : mOwnedEntities)
    {
        handle.get(aPhase)->erase();
    }
    mOwnedEntities.clear();
}


void SplashScene::update(const snac::Time & aTime, RawInput & aInput)
{
    TIME_RECURRING_CLASSFUNC(Main);

    mTimeAccu += aTime.mDeltaDuration;
    constexpr snac::Clock::duration gDuration{
        std::chrono::duration_cast<snac::Clock::duration>(
            std::chrono::seconds{2})};
    constexpr snac::Clock::duration gFadeDuration{
        std::chrono::duration_cast<snac::Clock::duration>(
            std::chrono::milliseconds{300})};

    if (mTimeAccu >= gDuration)
    {
        transitionToMenu();
        return;
    }
    else
    {
        float opacity = 
            float(
                snac::asSeconds(std::min(std::min(mTimeAccu, gFadeDuration), gDuration - mTimeAccu))
                / snac::asSeconds(gFadeDuration));
        setOpacity(math::ease::SmoothStep<float>{}.ease(opacity));
    }

    // Resolve scene graph
    mSystems.get()->get<system::SceneGraphResolver>().update();
    return;
}


void SplashScene::setOpacity(float aOpacity)
{
    ent::Phase update;
    mSplashEntity.get(update)->get<component::GlobalPose>().mColor.a() = aOpacity;
}


void SplashScene::transitionToMenu()
{
    mGameContext.mSceneStack->popScene({.mTransitionType = TransType::QuitToMenu});

    /* // Add permanent game title */
    /* makeText(mGameContext, init, "Snacman", */
    /*          mGameContext.mResources.getFont("fonts/TitanOne-Regular.ttf"),
    */
    /*          math::hdr::gYellow<float>, {-0.25f, 0.75f}, {1.8f, 1.8f}); */
    mGameContext.mResources.getBlueprint("blueprints/decor.json", mGameContext.mWorld, mGameContext);

    mGameContext.mSceneStack->pushScene(
        std::make_shared<scene::StageDecorScene>(mGameContext,
                                                mMappingContext),
        {.mTransitionType = snacgame::scene::TransType::FirstLaunch}
    );

    mGameContext.mSceneStack->pushScene(
        std::make_shared<scene::MenuScene>(mGameContext, mMappingContext)
    );
}


} // namespace scene
} // namespace snacgame
} // namespace ad
