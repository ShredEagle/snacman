#include "entity/Entity.h"
#include "entity/Wrap.h"
#include "snacman/EntityUtilities.h"
#include "snacman/simulations/snacgame/component/Context.h"
#include "snacman/simulations/snacgame/Entities.h"
#include "snacman/simulations/snacgame/GameContext.h"
#include "snacman/simulations/snacgame/scene/Scene.h"
#include "snacman/simulations/snacgame/SceneGraph.h"

#include "../system/SceneStack.h"

namespace ad {
namespace snacgame {
namespace scene {
class StageDecorScene : public Scene
{
public:
    StageDecorScene(GameContext & aGameContext,
                    ent::Wrap<component::MappingContext> & aContext) :
        Scene("decor scene", aGameContext, aContext),
        mStageDecor{aGameContext.mWorld.createFromBlueprint(
            aGameContext.mResources.getBlueprint(
                "blueprints/decor.json", mGameContext.mWorld, mGameContext),
            "decor")}
    {
        insertEntityInScene(mStageDecor, mGameContext.mSceneRoot);
    }

    void onExit(Transition aTransition) override
    {
        if (aTransition.mTransitionType == TransType::QuitAll)
        {
            ent::Phase destroy;
            mStageDecor.get(destroy)->erase();
            mGameContext.mSoundManager.stopSound(mPlayingMusicCue);
        }
    }

    void onEnter(Transition aTransition) override
    {
        if (aTransition.mTransitionType == TransType::QuitAll)
        {
            mGameContext.mSceneStack->popScene(aTransition);
        }

        if (aTransition.mTransitionType == TransType::FirstLaunch)
        {
            sounds::SoundManager & soundManager = mGameContext.mSoundManager;
            handy::StringId music = soundManager.createStreamedOggData(
                *mGameContext.mResources.find("audio/music/Snacman-Level-V1.ogg"));

            sounds::Handle<ad::sounds::SoundCue> musicCue = 
                soundManager.createSoundCue(
                    {
                        {music, {/*loop*/-1}},
                    },
                    SoundCategory_Music,
                    ad::sounds::HIGHEST_PRIORITY
                );

            mPlayingMusicCue = soundManager.playSound(musicCue);
        }
    }

    void update(const snac::Time & aTime, RawInput & aInput) override {}

    ent::Handle<ent::Entity> mStageDecor;
    sounds::Handle<ad::sounds::PlayingSoundCue> mPlayingMusicCue;
};
} // namespace scene
} // namespace snacgame
} // namespace ad
