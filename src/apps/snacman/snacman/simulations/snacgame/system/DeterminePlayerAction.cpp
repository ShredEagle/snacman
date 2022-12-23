#include "DeterminePlayerAction.h"

#include "math/Color.h"
#include "snacman/Input.h"
#include "snacman/simulations/snacgame/ActionKeyMapper.h"
#include "snacman/simulations/snacgame/component/AllowedMovement.h"
#include "snacman/simulations/snacgame/component/Geometry.h"

#include "../component/Controller.h"
#include "../component/Level.h"

namespace ad {
namespace snacgame {
namespace system {

constexpr float gTurningZoneHalfWidth = 0.1f;

void DeterminePlayerAction::update(const snac::Input & aInput)
{
    ent::Phase nomutation;
    auto levelGrid = mLevel.get(nomutation)->get<component::Level>().mLevelGrid;
    int colCount = mLevel.get(nomutation)->get<component::Level>().mColCount;
    mPlayer.each([this, &aInput, colCount, &levelGrid, &nomutation](
                     component::Controller & aController,
                     component::Geometry & aPlayerGeometry,
                     component::PlayerMoveState & aPlayerMoveState) {
        int allowedMovementFlag = component::gAllowedMovementNone;
        int oldMoveState = aPlayerMoveState.mMoveState;
        int inputMoveFlag = gPlayerMoveFlagNone;

        auto pathUnderPlayerAllowedMove =
            levelGrid
                .at(aPlayerGeometry.mGridPosition.x()
                    + aPlayerGeometry.mGridPosition.y() * colCount)
                .get(nomutation)
                ->get<component::AllowedMovement>();

        if (aPlayerGeometry.mSubGridPosition.y() > 0.f
            || (pathUnderPlayerAllowedMove.mAllowedMovement
                    & component::gAllowedMovementUp
                && aPlayerGeometry.mSubGridPosition.x() > -gTurningZoneHalfWidth
                && aPlayerGeometry.mSubGridPosition.x()
                       < gTurningZoneHalfWidth))
        {
            allowedMovementFlag |= component::gAllowedMovementUp;
        }
        if (aPlayerGeometry.mSubGridPosition.y() < 0.f
            || (pathUnderPlayerAllowedMove.mAllowedMovement
                    & component::gAllowedMovementDown
                && aPlayerGeometry.mSubGridPosition.x() > -gTurningZoneHalfWidth
                && aPlayerGeometry.mSubGridPosition.x()
                       < gTurningZoneHalfWidth))
        {
            allowedMovementFlag |= component::gAllowedMovementDown;
        }
        if (aPlayerGeometry.mSubGridPosition.x() < 0.f
            || (pathUnderPlayerAllowedMove.mAllowedMovement
                    & component::gAllowedMovementLeft
                && aPlayerGeometry.mSubGridPosition.y() > -gTurningZoneHalfWidth
                && aPlayerGeometry.mSubGridPosition.y()
                       < gTurningZoneHalfWidth))
        {
            allowedMovementFlag |= component::gAllowedMovementLeft;
        }
        if (aPlayerGeometry.mSubGridPosition.x() > 0.f
            || (pathUnderPlayerAllowedMove.mAllowedMovement
                    & component::gAllowedMovementRight
                && aPlayerGeometry.mSubGridPosition.y() > -gTurningZoneHalfWidth
                && aPlayerGeometry.mSubGridPosition.y()
                       < gTurningZoneHalfWidth))
        {
            allowedMovementFlag |= component::gAllowedMovementRight;
        }

        if ((aPlayerGeometry.mSubGridPosition.y() < 0.f
             && !(allowedMovementFlag & component::gAllowedMovementUp))
            || (aPlayerGeometry.mSubGridPosition.y() > 0.f
                && !(allowedMovementFlag & component::gAllowedMovementDown)))
        {
            aPlayerGeometry.mSubGridPosition.y() = 0.f;
        }
        if ((aPlayerGeometry.mSubGridPosition.x() > 0.f
             && !(allowedMovementFlag & component::gAllowedMovementLeft))
            || (aPlayerGeometry.mSubGridPosition.x() < 0.f
                && !(allowedMovementFlag & component::gAllowedMovementRight)))
        {
            aPlayerGeometry.mSubGridPosition.x() = 0.f;
        }

        switch (aController.mType)
        {
        case component::ControllerType::Keyboard:
            // TODO: mKeyboard.mKeyState should not be a super long array
            for (std::size_t i = 0; i != aInput.mKeyboard.mKeyState.size(); ++i)
            {
                if (aInput.mKeyboard.mKeyState.at(i))
                {
                    int moveFlagcandidate = mKeyboardMapping.get(i);

                    if (moveFlagcandidate != gPlayerMoveFlagNone)
                    {
                        inputMoveFlag = moveFlagcandidate;
                    }
                }
            }
            break;
        case component::ControllerType::Gamepad:
            inputMoveFlag = gPlayerMoveFlagNone;
            break;
        default:
            break;
        }

        inputMoveFlag &= allowedMovementFlag;

        if (inputMoveFlag == gPlayerMoveFlagNone)
        {
            inputMoveFlag = oldMoveState & allowedMovementFlag;
        }

        aPlayerMoveState.mMoveState = inputMoveFlag;
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
