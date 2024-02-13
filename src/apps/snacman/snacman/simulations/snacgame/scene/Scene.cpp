#include "Scene.h"

#include "../Entities.h"
#include "../GameContext.h"

#include "../component/PlayerGameData.h"
#include "../component/PlayerSlot.h"

#include <entity/EntityManager.h>
#include <snac-renderer-V1/Camera.h>

namespace ad {
namespace snacgame {
namespace scene {

Scene::Scene(std::string aName,
             GameContext & aGameContext,
             ent::Wrap<component::MappingContext> & aContext) :
    mName{aName},
    mGameContext{aGameContext},
    mSystems{mGameContext.mWorld.addEntity()},
    mMappingContext{aContext},
    mCameraQuery{aGameContext.mWorld}
{}

Scene::Scene(Scene && aScene) noexcept :
    mName{std::move(aScene.mName)},
    mGameContext{aScene.mGameContext},
    mSystems{aScene.mSystems},
    mMappingContext{aScene.mMappingContext},
    mCameraQuery{aScene.mCameraQuery}
{}

} // namespace scene
} // namespace snacgame
} // namespace ad
