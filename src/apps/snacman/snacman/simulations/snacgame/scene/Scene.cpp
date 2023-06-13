#include "Scene.h"

#include "../GameContext.h"

#include "../component/PlayerSlot.h"
#include "../component/PlayerGameData.h"

namespace ad {
namespace snacgame {
namespace scene {

Scene::Scene(std::string aName,
      GameContext & aGameContext,
      EntityWrap<component::MappingContext> & aContext,
      ent::Handle<ent::Entity> aSceneRoot) :
    mName{aName},
    mSceneRoot{aSceneRoot},
    mGameContext{aGameContext},
    mSystems{mGameContext.mWorld.addEntity()},
    mContext{aContext}
{}

}
} // namespace snacgame
} // namespace ad
