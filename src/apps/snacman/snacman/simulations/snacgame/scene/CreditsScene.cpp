#include "CreditsScene.h"

#include "../GameContext.h"
#include "../typedef.h"

#include "../component/Text.h"
#include "../component/Controller.h"
#include "../component/Geometry.h"
#include "../component/GlobalPose.h"

#include "../system/SceneStack.h"

#include <snacman/Profiling.h>

#include <snacman/simulations/snacgame/Entities.h>
#include <snacman/simulations/snacgame/InputCommandConverter.h>

#include <snacman/simulations/snacgame/system/InputProcessor.h>
#include <snacman/simulations/snacgame/system/SceneGraphResolver.h>

#include <handy/StringUtilities.h>


namespace ad {
namespace snacgame {
namespace scene {

namespace {

    using Section = std::pair<std::string, std::vector<const char*>>;
    std::vector<Section> gCredits{
        {"Engine programers", {"Adrien David", "François Poizat"}},
        {"Gameplay programers", {"François Poizat", "Adrien David"}},
        {"3D artists", {"François Poizat", "Clara Soro-Bablet"}},
        {"Music", {"Maxime Barnier"}},
        {"Sfx", {"Adrien David", "Mallorie Di Ruscio"}},
    };

} // unnamed namespace

void CreditsScene::populateSection(std::size_t aSectionIdx)
{
    auto font = mGameContext.mResources.getFont("fonts/TitanOne-Regular.ttf");

    Phase init;

    clearEntities(init);

    mSectionTitle = makeText(
        mGameContext,
        init,
        gCredits.at(aSectionIdx).first,
        font,
        math::hdr::gYellow<float>,
        math::Position<2, float>{-0.5f, 0.1f},
        math::Size<2, float>{1.25f, 1.25f}
    );

    mOwnedEntities.push_back(mSectionTitle);

    mNames.clear();
    std::size_t nameIdx = 0;
    for (const char * name : gCredits.at(aSectionIdx).second)
    {
        mNames.push_back(
            makeText(
                mGameContext,
                init,
                name,
                font,
                math::hdr::gWhite<float>,
                math::Position<2, float>{-0.3f, -0.15f - nameIdx * 0.17f},
                math::Size<2, float>{1.1f, 1.1f}
            ));
        ++nameIdx;
        mOwnedEntities.push_back(mNames.back());
    }
}


CreditsScene::CreditsScene(GameContext & aGameContext,
                     ent::Wrap<component::MappingContext> & aContext) :
    Scene(gCreditsSceneName, aGameContext, aContext)
{
    populateSection(0);
    setOpacity(0.f);
}

void CreditsScene::onEnter(Transition) {
    // There is 1 transition possible
    // From the main menu
    mSystems = mGameContext.mWorld.addEntity("System for Credits");
    ent::Phase onEnter;
    mSystems.get(onEnter)->add(
        system::SceneGraphResolver{mGameContext, mGameContext.mSceneRoot})
        .add(system::InputProcessor{mGameContext, mMappingContext});

    mTimeAccu = snac::Clock::duration{0};
    mCurrentSection = 0;
}

void CreditsScene::onExit(Transition)
{
    // There is 1 transition possible
    // To the main menu
    ent::Phase destroy;
    clearEntities(destroy);
    mSystems.get(destroy)->erase();
}


void CreditsScene::clearEntities(ent::Phase & aPhase)
{
    for (auto handle : mOwnedEntities)
    {
        handle.get(aPhase)->erase();
    }
    mOwnedEntities.clear();
}


void CreditsScene::update(const snac::Time & aTime, RawInput & aInput)
{
    TIME_RECURRING_CLASSFUNC(Main);
    
    // Handle commands
    std::vector<ControllerCommand> controllerCommands =
        mSystems.get()->get<system::InputProcessor>().mapControllersInput(
            aInput, "unbound", "unbound");

    int accumulatedCommand = 0;
    for (auto controller : controllerCommands)
    {
        accumulatedCommand |= controller.mInput.mCommand;
    }

    if(accumulatedCommand & gQuitCommand)
    {
        mGameContext.mSceneStack->popScene({.mTransitionType = TransType::QuitToMenu});
        return;
    }

    // Handle rolling credits
    constexpr snac::Clock::duration gSectionDuration{
        std::chrono::duration_cast<snac::Clock::duration>(
            std::chrono::seconds{3})};

    constexpr snac::Clock::duration gFadeDuration{
        std::chrono::duration_cast<snac::Clock::duration>(
            std::chrono::milliseconds{450})};

    mTimeAccu += aTime.mDeltaDuration;
    
    if (mTimeAccu >= gSectionDuration)
    {
        mCurrentSection += mTimeAccu / gSectionDuration;
        mCurrentSection %= gCredits.size();
        populateSection(mCurrentSection);
        mTimeAccu %= gSectionDuration;
    }
    float opacity = 
        float(
            snac::asSeconds(std::min(std::min(mTimeAccu, gFadeDuration), gSectionDuration - mTimeAccu))
            / snac::asSeconds(gFadeDuration));
    setOpacity(opacity);

    // Resolve scene graph
    mSystems.get()->get<system::SceneGraphResolver>().update();
    return;
}


void CreditsScene::setOpacity(float aOpacity)
{
    ent::Phase update;
    mSectionTitle.get(update)->get<component::Text>().mColor.a() = aOpacity;
    for(auto entity : mNames)
    {
        entity.get(update)->get<component::Text>().mColor.a() = aOpacity;
    }
}


} // namespace scene
} // namespace snacgame
} // namespace ad
