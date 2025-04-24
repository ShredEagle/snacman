#include "PodiumScene.h"

#include "GameScene.h"
#include "JoinGameScene.h"
#include "MenuScene.h"
#include "Scene.h"
#include "snacman/simulations/snacgame/InputConstants.h"
#include "snacman/simulations/snacgame/component/Geometry.h"
#include "snacman/simulations/snacgame/system/AdvanceAnimations.h"
#include "../SceneGraph.h"
#include "../component/PlayerSlot.h"
#include "../component/GlobalPose.h"
#include "../component/Controller.h"
#include "../component/RigAnimation.h"

#include <snacman/EntityUtilities.h>
#include <snacman/QueryManipulation.h>

#include "../system/InputProcessor.h"
#include "../system/MenuManager.h"
#include "../system/SceneGraphResolver.h"
#include "../system/SceneStack.h"

#include "../ModelInfos.h"
#include "../Entities.h"
#include "../GameContext.h"
#include "../GameParameters.h"
#include "../component/Context.h"

#include <snacman/Profiling.h>

#include <math/Vector.h>

namespace ad {
namespace snacgame {
namespace scene {

constexpr math::Spherical<float> gPodiumInitialCameraSpherical{
    9.f,
    math::Radian<float>{1.241f},
    math::Radian<float>{0.558f}
};

constexpr math::Position<3, float> gPodiumInitialCameraPos{
    -2.f, 1.f, 0.f
};

PodiumScene::PodiumScene(GameContext & aGameContext,
                         ent::Wrap<component::MappingContext> & aMappingContext) :
    Scene(gPodiumSceneName, aGameContext, aMappingContext),
    mItems{aGameContext.mWorld},
    mSlots(aGameContext.mWorld)
{
    TIME_SINGLE(Main, "Constructor game scene");
    {
        TIME_SINGLE(Main, "Load game assets");
        mGameContext.mResources.getModel("models/podium/podium.sel",
                                         gMeshGenericEffect);
        mGameContext.mResources.getFont("fonts/fill_boiga.ttf", gTextSize);
    }

    ent::Phase init;
    mOwnedEntities.push_back(createMenuItem(mGameContext,
                   init,
                   "Play again",
                   mGameContext.mResources.getFont("fonts/fill_boiga.ttf", gTextSize),
                   math::Position<2, float>{-0.9f, -0.3f},
                   {
                       {gGoDown, "Change Players"}
                   },
                   Transition{.mTransitionType = TransType::GameFromPodium},
                   true,
                   {0.5f, 0.5f}));
    mOwnedEntities.push_back(createMenuItem(mGameContext,
                   init,
                   "Change Players",
                   mGameContext.mResources.getFont("fonts/fill_boiga.ttf", gTextSize),
                   math::Position<2, float>{-0.9f, -0.5f},
                   {
                       {gGoUp, "Play again"},
                       {gGoDown, "Quit"}
                   },
                   Transition{.mTransitionType = TransType::JoinGameFromPodium},
                   false,
                   {0.5f, 0.5f}));
    mOwnedEntities.push_back(createMenuItem(mGameContext,
                   init,
                   "Quit",
                   mGameContext.mResources.getFont("fonts/fill_boiga.ttf", gTextSize),
                   math::Position<2, float>{-0.9f, -0.7f},
                   {
                       {gGoUp, "Change Players"}
                   },
                   Transition{.mTransitionType = TransType::QuitToMenu},
                   false,
                   {0.5f, 0.5f}));

    mOwnedEntities.push_back(createPodium(mGameContext, mGameContext.mSceneRoot));

    ent::Handle<ent::Entity> camera = snac::getFirstHandle(mCameraQuery);
    renderer::Orbital & camOrbital =
        snac::getComponent<system::OrbitalCamera>(camera).mControl.mOrbital;
    camOrbital.mSpherical = gPodiumInitialCameraSpherical;
    camOrbital.mSphericalOrigin = gPodiumInitialCameraPos;
}

void PodiumScene::onEnter(Transition aTransition)
{
    ent::Phase init;
    mSystems = mGameContext.mWorld.addEntity("System for Podium");
    mSystems.get(init)
        ->add(system::SceneGraphResolver{mGameContext, mGameContext.mSceneRoot})
        .add(system::MenuManager{mGameContext})
        .add(system::AdvanceAnimations{mGameContext})
        .add(system::InputProcessor{mGameContext, mMappingContext});

    static const std::array<math::Quaternion<float>, 4> podiumOrientation{{
        math::Quaternion(math::UnitVec<3, float>{{0.f, 1.f, 0.f}},
                            math::Turn<float>{0.5f}),
        math::Quaternion(math::UnitVec<3, float>{{0.f, 1.f, 0.f}},
                            math::Turn<float>{0.55f}),
        math::Quaternion(math::UnitVec<3, float>{{0.f, 1.f, 0.f}},
                            math::Turn<float>{0.45f}),
        math::Quaternion(math::UnitVec<3, float>{{0.f, 1.f, 0.f}},
                            math::Turn<float>{0.5f}),
    }};
    constexpr std::array<math::Vec<3, float>, 4> podiumPos{{
        {0.f, 0.2f, 1.5f},
        {-2.1f, 0.f, 0.9f},
        {2.1f, 0.f, 0.5f},
        {40.f, 0.f, 0.f},
    }};
    const PodiumSceneInfo & info = std::get<PodiumSceneInfo>(aTransition.mSceneInfo);
    const RankingList & ranking = info.mRanking;
    for (std::size_t i = 0; i < ranking.playerCount; i++)
    {
        const RankingList::Rank & rank = ranking.ranking.at(i);
        component::PlayerSlot & slot = snac::getComponent<component::PlayerSlot>(rank.first);
        ent::Handle<ent::Entity> podiumPlayer = createPlayerModel(mGameContext, slot);
        component::Geometry & geo = snac::getComponent<component::Geometry>(podiumPlayer);
        geo.mOrientation *= podiumOrientation.at(i);
        geo.mPosition += podiumPos.at(i);
        mOwnedEntities.push_back(podiumPlayer);

        insertEntityInScene(podiumPlayer, mGameContext.mWorld.handleFromName("podium"));
    }
}

void PodiumScene::onExit(Transition aTransition)
{
    ent::Phase destroy;

    for (auto handle : mOwnedEntities)
    {
        handle.get(destroy)->erase();
    }

    mOwnedEntities.clear();
    mSystems.get(destroy)->erase();
}

void PodiumScene::update(const snac::Time & aTime, RawInput & aInput)
{
    std::vector<ControllerCommand> controllerCommands =
        mSystems.get()->get<system::InputProcessor>().mapControllersInput(
            aInput, "menu", "null");

    auto [menuItem, accumulatedCommand] =
        mSystems.get()->get<system::MenuManager>().manageMenu(
            controllerCommands);

    if (accumulatedCommand & gQuitCommand)
    {
        snac::eraseAllInQuery(mSlots);
        mGameContext.mSceneStack->popScene();
        mGameContext.mSceneStack->pushScene(
            std::make_shared<MenuScene>(mGameContext, mMappingContext),
            menuItem.mTransition
        );
        return;
    }

    if (accumulatedCommand & gSelectItem)
    {
        switch(menuItem.mTransition.mTransitionType)
        {
            case TransType::GameFromPodium:
                mGameContext.mSceneStack->popScene();
                mGameContext.mSceneStack->pushScene(
                    std::make_shared<GameScene>(mGameContext, mMappingContext),
                    menuItem.mTransition
                );
                return;
            case TransType::JoinGameFromPodium:
                mGameContext.mSceneStack->popScene();
                mGameContext.mSceneStack->pushScene(
                    std::make_shared<JoinGameScene>(mGameContext, mMappingContext),
                    menuItem.mTransition
                );
                return;
            case TransType::QuitToMenu:
                snac::eraseAllInQuery(mSlots);
                mGameContext.mSceneStack->popScene();
                mGameContext.mSceneStack->pushScene(
                    std::make_shared<MenuScene>(mGameContext, mMappingContext),
                    menuItem.mTransition
                );
                return;
            default:
                break;
        }
    }

    mSystems.get()->get<system::SceneGraphResolver>().update();
    mSystems.get()->get<system::AdvanceAnimations>().update(aTime);
}

}
}
}

