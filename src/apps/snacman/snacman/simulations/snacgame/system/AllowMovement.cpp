#include "AllowMovement.h"

#include "math/Color.h"
#include "snacman/Input.h"
#include "snacman/Logging.h"

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

        int intPosX = static_cast<int>(aGeo.mPosition.x() + 0.5f);
        int intPosY = static_cast<int>(aGeo.mPosition.y() + 0.5f);
        float fracPosX = aGeo.mPosition.x() - intPosX;
        float fracPosY = aGeo.mPosition.y() - intPosY;

        auto pathUnderPlayerAllowedMove =
            tiles.at(intPosX + intPosY * colCount);

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
