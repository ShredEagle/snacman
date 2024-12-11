#pragma once


#include <sounds/SoundManager.h>


namespace ad::snacgame {


struct GameContext;


enum SoundCategory : int
{
    SoundCategory_SFX,
    SoundCategory_Music,
};


/// @brief Model a SFX sound element of the game, that can be played by the game logic.
/// 
/// Currently, it plays only one instance of the sound at a time
/// (stopping the sound first if it is already playing).
/// This was initially a workaround for a bug in Sounds lib, and is not required anymore.
/// (but we keep it around, so this guarantes less concurrent cues, without making an obvious difference).
struct Sfx
{
    Sfx(const filesystem::path & aRelativePath, GameContext & aGameContext);
    ~Sfx();

    void play();

    sounds::Handle<sounds::SoundCue> mSoundCue;
    sounds::Handle<sounds::PlayingSoundCue> mPlayingSoundCue;
    // TODO: do not host that here
    GameContext * mGameContext;
};


} // namespace ad::snacgame