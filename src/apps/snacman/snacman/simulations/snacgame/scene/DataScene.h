#include "entity/Entity.h"
#include "entity/Wrap.h"
#include "snacman/EntityUtilities.h"
#include "../system/SceneStack.h"
#include "snacman/simulations/snacgame/SceneGraph.h"
#include "snacman/simulations/snacgame/component/Context.h"
#include "snacman/simulations/snacgame/Entities.h"
#include "snacman/simulations/snacgame/GameContext.h"
#include "snacman/simulations/snacgame/scene/Scene.h"

namespace ad {
namespace snacgame {
namespace scene {
class StageDecorScene : public Scene
{
public:
    StageDecorScene(GameContext & aGameContext,
                    ent::Wrap<component::MappingContext> & aContext) :
        Scene("decor", aGameContext, aContext),
        mStageDecor{createStageDecor(aGameContext)}
    {
        insertEntityInScene(mStageDecor, mGameContext.mSceneRoot);
    }

    void onExit(Transition aTransition) override
    {
        if (aTransition.mTransitionName == sQuitTransition)
        {
            ent::Phase destroy;
            mStageDecor.get(destroy)->erase();
        }
    }

    void onEnter(Transition aTransition) override
    {
        if (aTransition.mTransitionName == sQuitTransition)
        {
            mGameContext.mSceneStack->popScene(aTransition);
        }
    }

    void update(const snac::Time & aTime, RawInput & aInput) override
    {
    }

    static constexpr char sQuitTransition[] = "Quit";

    ent::Handle<ent::Entity> mStageDecor;
};
} // namespace scene
} // namespace snacgame
} // namespace ad
