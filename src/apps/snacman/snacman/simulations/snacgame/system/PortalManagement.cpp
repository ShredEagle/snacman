#include "PortalManagement.h"

#include "../typedef.h"
#include "../LevelHelper.h"

#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {
void PortalManagement::update()
{
    TIME_RECURRING_CLASSFUNC(Main);
    EntHandle level = *mGameContext->mLevel;
    if (level.isValid())
    {
        ent::Phase portalPhase;
        const component::LevelData & levelData =
            level.get(portalPhase)->get<component::LevelData>();
        const std::vector<component::Tile> & tiles = levelData.mTiles;
        int colCount = levelData.mSize.height();

        mPlayer.each([&](component::Geometry & aPlayerGeo,
                         component::PlayerMoveState & aMoveState) {
            math::Position<3, float> & playerPos = aPlayerGeo.mPosition;
            const Pos2_i intPlayerPos = getLevelPosition_i(playerPos.xy());

            if (aMoveState.mCurrentPortal != -1 && aMoveState.mDestinationPortal != -1)
            {
                const component::Tile & currentTile = tiles.at(intPlayerPos.x() + intPlayerPos.y() * colCount);
                if (currentTile.mType == component::TileType::Void)
                {
                    int destFlatPosition = aMoveState.mDestinationPortal;
                    playerPos.x() = static_cast<float>(destFlatPosition % colCount);
                    playerPos.y() = static_cast<float>(destFlatPosition / colCount);
                }
            }

            aMoveState.mCurrentPortal = -1;
            aMoveState.mDestinationPortal = -1;

            mPortals.each([&](component::Geometry & aPortalGeo, component::Portal & aPortal) {
                math::Position<2, int> intPortalPos{aPortalGeo.mPosition.xy()};
                if (intPlayerPos == intPortalPos)
                {
                    int destinationPortalIndex = -1;
                    for (int portalIndex : levelData.mPortalIndex)
                    {
                        if (portalIndex != aPortal.portalIndex)
                        {
                            destinationPortalIndex = portalIndex;
                            break;
                        }
                    }

                    aMoveState.mCurrentPortal = aPortal.portalIndex;
                    aMoveState.mDestinationPortal = destinationPortalIndex;
                }
            });
        });
    }
}
} // namespace system
} // namespace snacgame
} // namespace ad
