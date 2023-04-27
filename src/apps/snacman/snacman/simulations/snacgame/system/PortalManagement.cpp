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

        mPlayer.each([&levelData, this](
                         EntHandle aPlayerHandle,
                         component::GlobalPose & aPlayerPose,
                         component::PlayerMoveState & aMoveState,
                         const component::Collision & aPlayerCol,
                         component::Geometry & aPlayerGeo,
                         component::PlayerPortalData & aPortalData) {
            aMoveState.mCurrentPortal = -1;
            aMoveState.mDestinationPortal = -1;

            Box_f playerHitbox = component::transformHitbox(
                aPlayerPose.mPosition, aPlayerCol.mHitbox);

            mPortals.each([&playerHitbox, &levelData, &aMoveState, &aPortalData, &aPlayerGeo](
                              const component::Portal & aPortal,
                              const component::GlobalPose & aPortalPose) {
                Box_f portalHitbox = component::transformHitbox(
                    aPortalPose.mPosition, aPortal.mEnterHitbox);

                DBGDRAW(snac::gPortalDrawer, snac::DebugDrawer::Level::off)
                    .addBox(
                        snac::DebugDrawer::Entry{
                            .mPosition = {0.f, 0.f, 0.f},
                            .mColor = math::hdr::gGreen<float>,
                        },
                        portalHitbox);

                Box_f portalExitHitbox = component::transformHitbox(
                    aPortalPose.mPosition, aPortal.mExitHitbox);

                DBGDRAW(snac::gPortalDrawer, snac::DebugDrawer::Level::off)
                    .addBox(
                        snac::DebugDrawer::Entry{
                            .mPosition = {0.f, 0.f, 0.f},
                            .mColor = math::hdr::gRed<float>,
                        },
                        portalExitHitbox);

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
                }

                if (aPortalData.mPortalImage)
                {
                    Phase handlePortalImage;
                    component::Geometry imageGeo = aPortalData.mPortalImage->get(handlePortalImage)->get<component::Geometry>();
                    component::GlobalPose imagePose = aPortalData.mPortalImage->get(handlePortalImage)->get<component::GlobalPose>();
                    component::Collision imageCollision = aPortalData.mPortalImage->get(handlePortalImage)->get<component::Collision>();
                    Box_f playerPortalImageHitbox = component::transformHitbox(
                        imagePose.mPosition, imageCollision.mHitbox);

                    if (component::collideWithSat(portalExitHitbox, playerPortalImageHitbox))
                    {
                        aPlayerGeo.mPosition += imageGeo.mPosition.as<math::Vec>();
                        aPortalData.mPortalImage->get(handlePortalImage)->erase();
                        aPortalData.mPortalImage = std::nullopt;
                    }

                    if (component::collideWithSat(portalExitHitbox, playerHitbox))
                    {
                        aPortalData.mPortalImage->get(handlePortalImage)->erase();
                        aPortalData.mPortalImage = std::nullopt;
                    }
                }
            });

            mPortals.each([&aMoveState, &aPortalData, &aPlayerHandle,
                           &aPlayerGeo,
                           this](const component::Portal & aPortal) {
                if (aPortal.portalIndex == aMoveState.mDestinationPortal
                    && !aPortalData.mPortalImage)
                {
                    EntHandle newPortalImage = mGameContext->mWorld.addEntity();
                    {
                        Phase addPortalImage;
                        Entity portal = *newPortalImage.get(addPortalImage);
                        Vec2 relativePos = aPortal.mMirrorSpawnPosition.xy()
                                           - aPlayerGeo.mPosition.xy();
                        addMeshGeoNode(
                            addPortalImage, *mGameContext, portal,
                            "models/donut/donut.gltf",
                            {relativePos.x(), relativePos.y(), 0.f}, 0.2f,
                            {1.f, 1.f, 1.f},
                            Quat_f{math::UnitVec<3, float>{{1.f, 0.f, 0.f}},
                                   math::Turn<float>{0.25f}});
                        portal.add(component::Collision{component::gPlayerHitbox});
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
