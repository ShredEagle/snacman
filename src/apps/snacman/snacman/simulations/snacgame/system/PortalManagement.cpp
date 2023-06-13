#include "PortalManagement.h"

#include "SceneGraphResolver.h"

#include "../component/GlobalPose.h"
#include "../component/Portal.h"
#include "../component/PlayerRoundData.h"
#include "../component/Collision.h"
#include "../component/SceneNode.h"
#include "../component/Geometry.h"
#include "../component/LevelData.h"
#include "../component/VisualModel.h"
#include "../component/PlayerSlot.h"
#include "../component/PlayerGameData.h"

#include "../GameContext.h"
#include "../Entities.h"
#include "../SceneGraph.h"
#include "../LevelHelper.h"
#include "../typedef.h"

#include <snacman/DebugDrawing.h>
#include <snacman/EntityUtilities.h>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {

PortalManagement::PortalManagement(GameContext & aGameContext) :
    mGameContext{&aGameContext},
    mPlayer{mGameContext->mWorld},
    mPortals{mGameContext->mWorld}
{}

void PortalManagement::preGraphUpdate()
{
    TIME_RECURRING_CLASSFUNC(Main);
    EntHandle level = *mGameContext->mLevel;
    if (level.isValid())
    {
        ent::Phase portalPhase;

        mPlayer.each([this](EntHandle aPlayerHandle,
                            component::PlayerRoundData & aRoundData) {
            mPortals.each([&aPlayerHandle, &aRoundData,
                           this](const component::Portal & aPortal) {
                if (aPortal.portalIndex == aRoundData.mDestinationPortal
                    && !aRoundData.mPortalImage)
                {
                    EntHandle newPortalImage = createPortalImage(
                        *mGameContext, aRoundData, aPortal);
                    insertEntityInScene(newPortalImage, aPlayerHandle);
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
        const component::Level & levelData =
            level.get(portalPhase)->get<component::Level>();

        mPlayer.each([&levelData, &portalPhase, this](
                         EntHandle aPlayerHandle,
                         component::GlobalPose & aPlayerPose,
                         const component::Collision & aPlayerCol,
                         component::Geometry & aPlayerGeo,
                         component::PlayerRoundData & aRoundData,
                         const component::SceneNode & aPlayerNode) {
            aRoundData.mCurrentPortal = -1;
            aRoundData.mDestinationPortal = -1;

            Box_f playerHitbox = component::transformHitbox(
                aPlayerPose.mPosition, aPlayerCol.mHitbox);

            mPortals.each([&playerHitbox, &levelData, &aPlayerGeo,
                           &aRoundData, &aPlayerNode, &portalPhase](
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

                    aRoundData.mCurrentPortal = aPortal.portalIndex;
                    aRoundData.mCurrentPortalPos = aPortalGeo.mPosition.xy();
                    aRoundData.mDestinationPortal = destinationPortalIndex;
                }

                if (aRoundData.mPortalImage)
                {
                    Phase handlePortalImage;
                    component::GlobalPose imagePose =
                        aRoundData.mPortalImage->get(handlePortalImage)
                            ->get<component::GlobalPose>();
                    component::Collision imageCollision =
                        aRoundData.mPortalImage->get(handlePortalImage)
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
                        updateGlobalPosition(aPlayerNode);
                        // TODO: (franz) this can be removed once the models can be made transparent
                        if (aRoundData.mModel.get()->has<component::VisualModel>())
                        {
                            snac::getComponent<component::VisualModel>(
                                aRoundData.mModel)
                                .mDisableInterpolation = true;
                        }
                        aRoundData.mCurrentPortal = -1;
                        aRoundData.mDestinationPortal = -1;

                        aRoundData.mPortalImage->get(portalPhase)
                            ->erase();
                        aRoundData.mPortalImage = std::nullopt;
                    }

                    if (component::collideWithSat(portalExitHitbox,
                                                  playerHitbox))
                    {
                        aRoundData.mCurrentPortal = -1;
                        aRoundData.mDestinationPortal = -1;

                        aRoundData.mPortalImage->get(portalPhase)
                            ->erase();
                        aRoundData.mPortalImage = std::nullopt;
                    }
                }
            });
        });
    }
}
} // namespace system
} // namespace snacgame
} // namespace ad
