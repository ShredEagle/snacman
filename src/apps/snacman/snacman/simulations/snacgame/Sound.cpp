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

void Sfx::play()
{
    mPlayingSoundCue = mGameContext->mSoundManager.playSound(mSoundCue);
}
 

} // namespace ad::snacgame