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
    ent::Phase portalPhase;

    mPlayer.each([this](EntHandle aPlayerHandle,
                        component::PlayerRoundData & aRoundData) {
        mPortals.each([&aPlayerHandle, &aRoundData,
                       this](const component::Portal & aPortal) {
            if (aPortal.portalIndex == aRoundData.mDestinationPortal
                && !aRoundData.mPortalImage.isValid())
            {
                EntHandle newPortalImage = createPortalImage(
                    *mGameContext, aRoundData, aPortal);
                insertEntityInScene(newPortalImage, aPlayerHandle);
            }
        });
    });
}

void PortalManagement::postGraphUpdate(component::Level & aLevelData)
{
    TIME_RECURRING_CLASSFUNC(Main);
    ent::Phase portalPhase;

    mPlayer.each([&aLevelData, &portalPhase, this](
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

        mPortals.each([&playerHitbox, &aLevelData, &aPlayerGeo,
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
                for (int portalIndex : aLevelData.mPortalIndex)
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

            if (aRoundData.mPortalImage.isValid())
            {
                ent::Entity portalEnt = *aRoundData.mPortalImage.get(portalPhase);
                component::GlobalPose imagePose =
                    portalEnt.get<component::GlobalPose>();
                component::Collision imageCollision =
                    portalEnt.get<component::Collision>();
                Box_f playerPortalImageHitbox = component::transformHitbox(
                    imagePose.mPosition, imageCollision.mHitbox);

                DBGDRAW_DEBUG(snac::gPortalDrawer)
                    .addBox(
                        snac::DebugDrawer::Entry{
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

                    portalEnt.erase();
                }

                if (component::collideWithSat(portalExitHitbox,
                                              playerHitbox))
                {
                    aRoundData.mCurrentPortal = -1;
                    aRoundData.mDestinationPortal = -1;

                    portalEnt.erase();
                }
            }
        });
    });
}
} // namespace system
} // namespace snacgame
} // namespace ad
