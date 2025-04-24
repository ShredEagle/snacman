#include "AllowMovement.h"

#include "../LevelHelper.h"
#include "../typedef.h"
#include "../GameContext.h"
#include "../InputCommandConverter.h"

#include "../component/PlayerGameData.h"
#include "../component/PlayerSlot.h"
#include "../component/Geometry.h"
#include "../component/AllowedMovement.h"
#include "../component/LevelData.h"

#include <snacman/Input.h>
#include <snacman/Logging.h>
#include <snacman/Profiling.h>

#include <entity/EntityManager.h>

#include <math/Color.h>

#include <cmath>

namespace ad {
namespace snacgame {
namespace system {

AllowMovement::AllowMovement(GameContext & aGameContext) :
    mGameContext{&aGameContext},
    mMover{mGameContext->mWorld}
{}


void AllowMovement::update(component::Level & aLevelData)
{
    TIME_RECURRING_CLASSFUNC(Main);

    ent::Phase nomutation;

    auto tiles = aLevelData.mTiles;
    if (tiles.size() == 0) [[unlikely]]
    {
        return;
    }
    int colCount = aLevelData.mSize.height();
    mMover.each([&](component::Geometry & aGeo,
                    component::AllowedMovement & aAllowedMovement) {
        int allowedMovementFlag = gAllowedMovementNone;

        Pos2_i intPos = getLevelPosition_i(aGeo.mPosition.xy());
        float fracPosX = aGeo.mPosition.x() - intPos.x();
        float fracPosY = aGeo.mPosition.y() - intPos.y();

        auto pathUnderPlayerAllowedMove =
            tiles.at(intPos.x() + intPos.y() * colCount);

        if (fracPosY < 0.f
            || (pathUnderPlayerAllowedMove.mAllowedMove
                    & gAllowedMovementUp
                && fracPosX > -aAllowedMovement.mWindow
                && fracPosX < aAllowedMovement.mWindow))
        {
            allowedMovementFlag |= gAllowedMovementUp;
        }
        if (fracPosY > 0.f
            || (pathUnderPlayerAllowedMove.mAllowedMove
                    & gAllowedMovementDown
                && fracPosX > -aAllowedMovement.mWindow
                && fracPosX < aAllowedMovement.mWindow))
        {
            allowedMovementFlag |= gAllowedMovementDown;
        }
        if (fracPosX > 0.f
            || (pathUnderPlayerAllowedMove.mAllowedMove
                    & gAllowedMovementLeft
                && fracPosY > -aAllowedMovement.mWindow
                && fracPosY < aAllowedMovement.mWindow))
        {
            allowedMovementFlag |= gAllowedMovementLeft;
        }
        if (fracPosX < 0.f
            || (pathUnderPlayerAllowedMove.mAllowedMove
                    & gAllowedMovementRight
                && fracPosY > -aAllowedMovement.mWindow
                && fracPosY < aAllowedMovement.mWindow))
        {
            allowedMovementFlag |= gAllowedMovementRight;
        }
        aAllowedMovement.mAllowedMovement = allowedMovementFlag;
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
