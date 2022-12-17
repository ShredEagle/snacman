#include "DeterminePlayerAction.h"

#include "math/Color.h"
#include "snacman/Input.h"
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
    int colCount =
        mLevel.get(nomutation)->get<component::Level>().mColCount;
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

        if (action == gPlayerMoveDown)
        {
            bool isPath =
                levelGrid
                    .at(aPlayerGeometry.mGridPosition.x()
                        + (aPlayerGeometry.mGridPosition.y() + 1) * colCount)
                    .get(nomutation)
                    ->has<component::Geometry>();
            if ((!isPath && aPlayerGeometry.mSubGridPosition.y() >= 0.f) || (
                    aPlayerGeometry.mSubGridPosition.x() > 0.2f && aPlayerGeometry.mSubGridPosition.x() < -0.2f
                       ))
            {
                aPlayerGeometry.mSubGridPosition.y() = 0.f;
                action = -1;
            }
            else
            {
                aPlayerGeometry.mSubGridPosition.x() = 0.f;
            }
        }
        else if (action == gPlayerMoveUp)
        {
            bool isPath =
                levelGrid
                    .at(aPlayerGeometry.mGridPosition.x()
                        + (aPlayerGeometry.mGridPosition.y() - 1) * colCount)
                    .get(nomutation)
                    ->has<component::Geometry>();
            if ((!isPath && aPlayerGeometry.mSubGridPosition.y() <= 0.f) || (
                    aPlayerGeometry.mSubGridPosition.x() > 0.2f && aPlayerGeometry.mSubGridPosition.x() < -0.2f
                       ))
            {
                aPlayerGeometry.mSubGridPosition.y() = 0.f;
                action = -1;
            }
            else
            {
                aPlayerGeometry.mSubGridPosition.x() = 0.f;
            }
        }
        else if (action == gPlayerMoveRight)
        {
            bool isPath =
                levelGrid
                    .at((aPlayerGeometry.mGridPosition.x() - 1)
                        + aPlayerGeometry.mGridPosition.y() * colCount)
                    .get(nomutation)
                    ->has<component::Geometry>();
            if ((!isPath && aPlayerGeometry.mSubGridPosition.x() <= 0.f) || (
                    aPlayerGeometry.mSubGridPosition.y() > 0.2f && aPlayerGeometry.mSubGridPosition.y() < -0.2f
                       ))
            {
                aPlayerGeometry.mSubGridPosition.y() = 0.f;
                action = -1;
            }
        }
        else if (action == gPlayerMoveLeft)
        {
            bool isPath =
                levelGrid
                    .at((aPlayerGeometry.mGridPosition.x() + 1)
                        + aPlayerGeometry.mGridPosition.y() * colCount)
                    .get(nomutation)
                    ->has<component::Geometry>();
            if ((!isPath && aPlayerGeometry.mSubGridPosition.x() >= 0.f) || (
                    aPlayerGeometry.mSubGridPosition.y() > 0.2f && aPlayerGeometry.mSubGridPosition.y() < -0.2f
                       ))
            {
                aPlayerGeometry.mSubGridPosition.x() = 0.f;
                action = -1;
            }
            else
            {
                aPlayerGeometry.mSubGridPosition.y() = 0.f;
            }
        }
        aPlayerMoveState.mMoveState = action;
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
