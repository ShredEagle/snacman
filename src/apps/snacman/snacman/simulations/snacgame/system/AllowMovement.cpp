#include "AllowMovement.h"

#include "math/Color.h"
#include "snacman/Input.h"
#include "snacman/Logging.h"
#include "snacman/simulations/snacgame/LevelHelper.h"

#include "../typedef.h"

#include "../component/AllowedMovement.h"
#include "../component/LevelData.h"

#include <cmath>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {

void AllowMovement::update()
{
    TIME_RECURRING_CLASSFUNC(Main);

    ent::Phase nomutation;

    component::LevelData & aLevelData = mGameContext->mLevel->get(nomutation)->get<component::LevelData>();
    auto tiles = aLevelData.mTiles;
    if (tiles.size() == 0) [[unlikely]]
    {
        return;
    }
    int colCount = aLevelData.mSize.height();
    mMover.each([&](component::Geometry & aGeo,
                    component::AllowedMovement & aAllowedMovement) {
        int allowedMovementFlag = component::gAllowedMovementNone;

        Pos2_i intPos = getLevelPosition_i(aGeo.mPosition.xy());
        float fracPosX = aGeo.mPosition.x() - intPos.x();
        float fracPosY = aGeo.mPosition.y() - intPos.y();

        auto pathUnderPlayerAllowedMove =
            tiles.at(intPos.x() + intPos.y() * colCount);

        if (fracPosY < 0.f
            || (pathUnderPlayerAllowedMove.mAllowedMove
                    & component::gAllowedMovementUp
                && fracPosX > -aAllowedMovement.mWindow
                && fracPosX < aAllowedMovement.mWindow))
        {
            allowedMovementFlag |= component::gAllowedMovementUp;
        }
        if (fracPosY > 0.f
            || (pathUnderPlayerAllowedMove.mAllowedMove
                    & component::gAllowedMovementDown
                && fracPosX > -aAllowedMovement.mWindow
                && fracPosX < aAllowedMovement.mWindow))
        {
            allowedMovementFlag |= component::gAllowedMovementDown;
        }
        if (fracPosX > 0.f
            || (pathUnderPlayerAllowedMove.mAllowedMove
                    & component::gAllowedMovementLeft
                && fracPosY > -aAllowedMovement.mWindow
                && fracPosY < aAllowedMovement.mWindow))
        {
            allowedMovementFlag |= component::gAllowedMovementLeft;
        }
        if (fracPosX < 0.f
            || (pathUnderPlayerAllowedMove.mAllowedMove
                    & component::gAllowedMovementRight
                && fracPosY > -aAllowedMovement.mWindow
                && fracPosY < aAllowedMovement.mWindow))
        {
            allowedMovementFlag |= component::gAllowedMovementRight;
        }
        aAllowedMovement.mAllowedMovement = allowedMovementFlag;
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
