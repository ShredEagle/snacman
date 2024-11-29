#pragma once


#include <sounds/SoundManager.h>


namespace ad::snacgame {


struct GameContext;


enum SoundCategory : int
{
    SoundCategory_SFX,
    SoundCategory_Music,
};


struct Sfx
{
    Sfx(const filesystem::path & aRelativePath, GameContext & aGameContext);

    void play();

    sounds::Handle<sounds::SoundCue> mSoundCue;
    sounds::Handle<sounds::PlayingSoundCue> mPlayingSoundCue;
    // TODO: do not host that here
    GameContext * mGameContext;
};


} // namespace ad::snacgame