#include "DeterminePlayerAction.h"

#include "math/Color.h"
#include "snacman/Input.h"
#include "snacman/simulations/snacgame/component/AllowedMovement.h"
#include "snacman/simulations/snacgame/component/Geometry.h"

#include "../component/Controller.h"
#include "../component/Level.h"

namespace ad {
namespace snacgame {
namespace system {

void DeterminePlayerAction::update(const snac::Input & aInput)
{
    ent::Phase nomutation;
    auto levelGrid = mLevel.get(nomutation)->get<component::Level>().mLevelGrid;
    int colCount = mLevel.get(nomutation)->get<component::Level>().mColCount;
    mPlayer.each([this, &aInput, colCount, &levelGrid, &nomutation](
                     component::Controller & aController,
                     component::Geometry & aPlayerGeometry,
                     component::PlayerMoveState & aPlayerMoveState) {
        PlayerActionFlag action = aPlayerMoveState.mMoveState;

        switch (aController.mType)
        {
        case component::ControllerType::Keyboard:
            // TODO: mKeyboard.mKeyState should not be a super long array
            for (std::size_t i = 0; i != aInput.mKeyboard.mKeyState.size(); ++i)
            {
                if (aInput.mKeyboard.mKeyState.at(i))
                {
                    PlayerActionFlag actionCandidate = mKeyboardMapping.get(i);

                    if (actionCandidate != -1)
                    {
                        action = actionCandidate;
                    }
                }
            }
            break;
        case component::ControllerType::Gamepad:
            action = -1;
            break;
        default:
            break;
        }

        auto pathUnderPlayerAllowedMove =
            levelGrid
                .at(aPlayerGeometry.mGridPosition.x()
                    + aPlayerGeometry.mGridPosition.y() * colCount)
                .get(nomutation)
                ->get<component::AllowedMovement>();

        // If the player wants to move down and is not allowed to move down
        if (!(pathUnderPlayerAllowedMove.mAllowedMovement
                & component::AllowedMovementDown)
            && aPlayerGeometry.mSubGridPosition.y() >= 0.f
            && action == gPlayerMoveDown)
        {
            aPlayerGeometry.mSubGridPosition.y() = 0.f;
            action = -1;
        }
        else if (!(pathUnderPlayerAllowedMove.mAllowedMovement
                     & component::AllowedMovementUp)
                 && aPlayerGeometry.mSubGridPosition.y() <= 0.f
                 && action == gPlayerMoveUp)
        {
            aPlayerGeometry.mSubGridPosition.y() = 0.f;
            action = -1;
        }
        else if (!(pathUnderPlayerAllowedMove.mAllowedMovement
                 & component::AllowedMovementLeft)
                 && aPlayerGeometry.mSubGridPosition.x() >= 0.f
                 && action == gPlayerMoveLeft)
        {
            aPlayerGeometry.mSubGridPosition.x() = 0.f;
            action = -1;
        }
        else if (!(pathUnderPlayerAllowedMove.mAllowedMovement
                 & component::AllowedMovementRight)
                 && aPlayerGeometry.mSubGridPosition.x() <= 0.f
                 && action == gPlayerMoveRight)
        {
            aPlayerGeometry.mSubGridPosition.x() = 0.f;
            action = -1;
        }

        aPlayerMoveState.mMoveState = action;
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
