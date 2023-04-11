#pragma once

#include "snacman/simulations/snacgame/GameParameters.h"
#include "typedef.h"

namespace ad {
namespace snacgame {

inline Pos2 getLevelPosition(const Pos2& aPos)
{
    return Pos2{std::floor(aPos.x() + gCellSize / 2), std::floor(aPos.y() + gCellSize / 2)};
}

inline Pos2_i getLevelPosition_i(const Pos2 & aPos)
{
    return Pos2_i{static_cast<int>(aPos.x() + gCellSize / 2), static_cast<int>(aPos.y() + gCellSize / 2)};
}

}
}
