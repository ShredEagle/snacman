#include "Sound.h"

#include "GameContext.h"


namespace ad::snacgame {



Sfx::Sfx(const filesystem::path & aRelativePath, GameContext & aGameContext) :
    mGameContext{&aGameContext}
{
    sounds::SoundManager & soundManager = aGameContext.mSoundManager;
    handy::StringId sound =
        soundManager.createData(*aGameContext.mResources.find(aRelativePath));
    mSoundCue = 
        soundManager.createSoundCue(
            {
                {sound, {/*loop*/0}},
            },
            SoundCategory_SFX,
            ad::sounds::HIGHEST_PRIORITY
        );
}

Sfx::~Sfx()
{
    // TODO Ad 2024/11/29: We should not have to manually destroy sounds, this is a temporary workaround
    if(mPlayingSoundCue.mHandleIndex != -1)
    {
        mGameContext->mSoundManager.stopSound(mPlayingSoundCue);
    }
}


void Sfx::play()
{
    // Ad 2024/11/29: #sound This is very bad, should be done automatically by soundmanager
    // only when sound is DONE playing!
    // ---
    // Note: This is now fixed in Sounds library, so the mPlayingSoundCue member and manual stopping
    // are not required anymore.
    // Please note that there is a maximal number of concurrent sources for a given cue (MAX_SOURCE_PER_CUE)
    if(mPlayingSoundCue.mHandleIndex != -1)
    {
        mGameContext->mSoundManager.stopSound(mPlayingSoundCue);
    }
    mPlayingSoundCue = mGameContext->mSoundManager.playSound(mSoundCue);
}
 

} // namespace ad::snacgame