#include "PortalManagement.h"

#include "snacman/simulations/snacgame/Entities.h"
#include "snacman/simulations/snacgame/SceneGraph.h"

#include "../LevelHelper.h"
#include "../typedef.h"

#include <snacman/DebugDrawing.h>
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

        mPlayer.each([&tiles, &colCount, &levelData, this](
                         EntHandle aPlayerHandle,
                         component::GlobalPose & aPlayerGeo,
                         component::PlayerMoveState & aMoveState,
                         const component::Collision & aPlayerCol,
                         component::PlayerPortalData & aPortalData) {
            math::Position<3, float> & playerPos = aPlayerGeo.mPosition;
            const Pos2_i intPlayerPos = getLevelPosition_i(playerPos.xy());

            // Player is on a portal
            if (aMoveState.mCurrentPortal != -1
                && aMoveState.mDestinationPortal != -1)
            {
                const component::Tile & currentTile =
                    tiles.at(intPlayerPos.x() + intPlayerPos.y() * colCount);
                // If the player step on a void path (meaning he is out of the
                // portal) we teleport him
                if (currentTile.mType == component::TileType::Void)
                {
                    int destFlatPosition = aMoveState.mDestinationPortal;
                    playerPos.x() =
                        static_cast<float>(destFlatPosition % colCount);
                    playerPos.y() =
                        static_cast<float>(destFlatPosition / colCount);
                }
            }

            aMoveState.mCurrentPortal = -1;
            aMoveState.mDestinationPortal = -1;

            Box_f playerHitbox = component::transformHitbox(
                aPlayerGeo.mPosition, aPlayerCol.mHitbox);

            mPortals.each([&playerHitbox, &levelData, &aMoveState,
                           &aPortalData](
                              const component::Portal & aPortal,
                              const component::GlobalPose & aPortalPose) {
                Box_f portalHitbox = component::transformHitbox(
                    aPortalPose.mPosition, aPortal.mEnterHitbox);

                DBGDRAW(snac::gPortalDrawer, snac::DebugDrawer::Level::off).addBox(
                    snac::DebugDrawer::Entry{
                        .mPosition = {0.f, 0.f, 0.f},
                        .mColor = math::hdr::gGreen<float>,
                    },
                    portalHitbox
                );

                Box_f portalExitHitbox = component::transformHitbox(
                    aPortalPose.mPosition, aPortal.mExitHitbox);

                DBGDRAW(snac::gPortalDrawer, snac::DebugDrawer::Level::off).addBox(
                    snac::DebugDrawer::Entry{
                        .mPosition = {0.f, 0.f, 0.f},
                        .mColor = math::hdr::gRed<float>,
                    },
                    portalExitHitbox
                );

                // If player collides with the portal hitbox we set up
                // the player portal teleportation
                if (component::collideWithSat(portalHitbox, playerHitbox))
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
                    aPortalData.mCurrentPortalPos = aPortalPose.mPosition.xy();
                }
            });

            mPortals.each([&aMoveState, &aPortalData, &aPlayerHandle, this](
                              const component::GlobalPose & aPortalGeo,
                              const component::Portal & aPortal) {
                if (aPortal.portalIndex == aMoveState.mDestinationPortal
                    && !aPortalData.mPortalImage)
                {
                    EntHandle newPortalImage = mGameContext->mWorld.addEntity();
                    {
                        Phase addPortalImage;
                        Entity portal = *newPortalImage.get(addPortalImage);
                        Vec2 relativePos = aPortalGeo.mPosition.xy()
                                           - aPortalData.mCurrentPortalPos;
                        addMeshGeoNode(
                            addPortalImage, *mGameContext, portal,
                            "models/donut/DONUT.gltf",
                            {relativePos.x(), relativePos.y(), 0.f}, 1.f,
                            {1.f, 1.f, 1.f},
                            Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                   math::Turn<float>{0.25f}});
                        aPortalData.mPortalImage = newPortalImage;
                    }
                    insertEntityInScene(newPortalImage, aPlayerHandle);
                }
            });
        });
    }
}
} // namespace system
} // namespace snacgame
} // namespace ad
