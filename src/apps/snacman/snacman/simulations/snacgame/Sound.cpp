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
    // TODO Ad 2024/11/29: #sound This is very bad, should be done automatically by soundmanager
    // only when sound is DONE playing!
    if(mPlayingSoundCue.mHandleIndex != -1)
    {
        mGameContext->mSoundManager.stopSound(mPlayingSoundCue);
    }
    mPlayingSoundCue = mGameContext->mSoundManager.playSound(mSoundCue);
}
 

} // namespace ad::snacgame