#include "PodiumScene.h"
#include "snacman/simulations/snacgame/GameContext.h"
#include "snacman/simulations/snacgame/GameParameters.h"
#include "snacman/simulations/snacgame/component/Context.h"


namespace ad {
namespace snacgame {
namespace scene {

PodiumScene::PodiumScene(GameContext & aGameContext,
                         ent::Wrap<component::MappingContext> & aMappingContext) :
    Scene(gPodiumSceneName, aGameContext, aMappingContext),
    mItems{aGameContext.mWorld}
{}

void PodiumScene::onEnter(Transition aTransition)
{}

void PodiumScene::onExit(Transition aTransition)
{}

void PodiumScene::update(const snac::Time & aTime, RawInput & aInput)
{}

}
}
}

