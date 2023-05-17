#include "PortalManagement.h"

#include "snacman/simulations/snacgame/component/PlayerModel.h"
#include "snacman/simulations/snacgame/Entities.h"
#include "snacman/simulations/snacgame/SceneGraph.h"

#include "../LevelHelper.h"
#include "../typedef.h"

#include <snacman/DebugDrawing.h>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {
void PortalManagement::preGraphUpdate()
{
    TIME_RECURRING_CLASSFUNC(Main);
    EntHandle level = *mGameContext->mLevel;
    if (level.isValid())
    {
        ent::Phase portalPhase;

        mPlayer.each([this](EntHandle aPlayerHandle,
                            component::PlayerModel & aPlayerModel,
                            component::PlayerPortalData & aPortalData) {
            mPortals.each([&aPortalData, &aPlayerHandle, &aPlayerModel,
                           this](const component::Portal & aPortal) {
                if (aPortal.portalIndex == aPortalData.mDestinationPortal
                    && !aPortalData.mPortalImage)
                {
                    EntHandle newPortalImage = mGameContext->mWorld.addEntity();
                    {
                        Phase addPortalImage;
                        component::Geometry modelGeo =
                            aPlayerModel.mModel.get(addPortalImage)
                                ->get<component::Geometry>();
                        Entity portal = *newPortalImage.get(addPortalImage);
                        Vec2 relativePos = aPortal.mMirrorSpawnPosition.xy()
                                           - aPortalData.mCurrentPortalPos;
                        addMeshGeoNode(*mGameContext, portal,
                                       "models/donut/donut.gltf",
                                       "effects/MeshTextures.sefx",
                                       {relativePos.x(), relativePos.y(), 0.f},
                                       modelGeo.mScaling,
                                       modelGeo.mInstanceScaling,
                                       modelGeo.mOrientation, modelGeo.mColor);
                        portal.add(
                            component::Collision{component::gPlayerHitbox});
                        aPortalData.mPortalImage = newPortalImage;
                    }
                    insertEntityInScene(newPortalImage, aPlayerHandle);
                }

                if (aPortalData.mCurrentPortal == -1
                    && aPortalData.mDestinationPortal == -1
                    && aPortalData.mPortalImage)
                {
                    Phase removePortalImage;
                    aPortalData.mPortalImage->get(removePortalImage)->erase();
                    aPortalData.mPortalImage = std::nullopt;
                }
            });
        });
    }
}

void PortalManagement::postGraphUpdate()
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
                         const component::Collision & aPlayerCol,
                         component::Geometry & aPlayerGeo,
                         component::PlayerModel & aPlayerModel,
                         component::PlayerPortalData & aPortalData) {
            aPortalData.mCurrentPortal = -1;
            aPortalData.mDestinationPortal = -1;

            Box_f playerHitbox = component::transformHitbox(
                aPlayerPose.mPosition, aPlayerCol.mHitbox);

            mPortals.each([&playerHitbox, &levelData, &aPortalData,
                           &aPlayerGeo](
                              const component::Portal & aPortal,
                              const component::Geometry & aPortalGeo,
                              const component::GlobalPose & aPortalPose) {
                Box_f portalHitbox = component::transformHitbox(
                    aPortalPose.mPosition, aPortal.mEnterHitbox);

                DBGDRAW(snac::gPortalDrawer, snac::DebugDrawer::Level::debug)
                    .addBox(
                        snac::DebugDrawer::Entry{
                            .mPosition = {0.f, 0.f, 0.f},
                            .mColor = math::hdr::gGreen<float>,
                        },
                        portalHitbox);

                Box_f portalExitHitbox = component::transformHitbox(
                    aPortalPose.mPosition, aPortal.mExitHitbox);

                DBGDRAW(snac::gPortalDrawer, snac::DebugDrawer::Level::debug)
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

                    aPortalData.mCurrentPortal = aPortal.portalIndex;
                    aPortalData.mCurrentPortalPos = aPortalGeo.mPosition.xy();
                    aPortalData.mDestinationPortal = destinationPortalIndex;
                }

                if (aPortalData.mPortalImage)
                {
                    Phase handlePortalImage;
                    component::GlobalPose imagePose =
                        aPortalData.mPortalImage->get(handlePortalImage)
                            ->get<component::GlobalPose>();
                    component::Collision imageCollision =
                        aPortalData.mPortalImage->get(handlePortalImage)
                            ->get<component::Collision>();
                    Box_f playerPortalImageHitbox = component::transformHitbox(
                        imagePose.mPosition, imageCollision.mHitbox);

                    DBGDRAW(snac::gPortalDrawer,
                            snac::DebugDrawer::Level::debug)
                        .addBox(
                            snac::DebugDrawer::Entry{
                                .mPosition = {0.f, 0.f, 0.f},
                                .mColor = math::hdr::gBlue<float>,
                            },
                            playerPortalImageHitbox);

                    if (component::collideWithSat(portalExitHitbox,
                                                  playerPortalImageHitbox))
                    {
                        aPlayerGeo.mPosition = aPortalGeo.mPosition;
                        aPortalData.mCurrentPortal = -1;
                        aPortalData.mDestinationPortal = -1;
                    }

                    if (component::collideWithSat(portalExitHitbox,
                                                  playerHitbox))
                    {
                        aPortalData.mCurrentPortal = -1;
                        aPortalData.mDestinationPortal = -1;
                    }
                }
            });
        });
    }
}
} // namespace system
} // namespace snacgame
} // namespace ad
