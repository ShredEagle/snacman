#pragma once

#include <entity/EntityManager.h>

#include <chrono>
#include <cstddef>
#include <optional>
#include <array>

namespace ad {
namespace snacgame {

constexpr std::size_t gNumberOfSavedStates = 64;

struct SaveState
{
    ent::State mState;
    std::chrono::microseconds mFrameDuration;
    std::chrono::microseconds mUpdateDelta;
};

struct SimulationControl
{
    bool mPlaying = true;
    bool mStep = false;
    bool mSaveGameState = true;
    float mSpeedFactor{1.f};
    std::size_t mSaveStateIndex = 0;
    int mSelectedSaveState = -1;
    std::array<std::optional<SaveState>, gNumberOfSavedStates>
        mPreviousGameState;

    void saveState(ent::State && aState,
                   std::chrono::microseconds aFrameDuration,
                   std::chrono::microseconds aUpdateDelta
                   )
    {
        mSaveStateIndex = (mSaveStateIndex + 1) % gNumberOfSavedStates;
        if (mSaveStateIndex == static_cast<std::size_t>(mSelectedSaveState))
        {
            mSelectedSaveState = -1;
        }
        mPreviousGameState.at(mSaveStateIndex) =
            SaveState{std::move(aState), aFrameDuration, aUpdateDelta};
    }

    void drawSimulationUi(ent::EntityManager & aWorld);
};

enum class VhsButtonType
{
    Play,
    Pause,
    FastForward,
    FastRewind,
    Step,
};

bool drawVhsButton(VhsButtonType aButtonType, const char * id);
} // namespace snacgame
} // namespace ad
