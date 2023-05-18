#include "EatPill.h"

#include "snacman/simulations/snacgame/component/GlobalPose.h"
#include "snacman/simulations/snacgame/component/PlayerHud.h"
#include <snacman/EntityUtilities.h>

#include "../component/SceneNode.h"
#include "../GameParameters.h"
#include "../typedef.h"

#include <entity/Entity.h>
#include <math/Homogeneous.h>
#include <math/Transformations.h>
#include <snacman/DebugDrawing.h>
#include <snacman/Profiling.h>


namespace ad {
namespace snacgame {
namespace system {


void EatPill::update()
{
    TIME_RECURRING_CLASSFUNC(Main);
    mPlayers.each([this](const component::GlobalPose & aPlayerGeo,
                         component::Collision aPlayerCol,
                         component::PlayerLifeCycle & aLifeCycle)
    {
        ent::Phase eatPillUpdate;
        Box_f playerHitbox = component::transformHitbox(aPlayerGeo.mPosition,
                                                        aPlayerCol.mHitbox);

        DBGDRAW(snac::gHitboxDrawer, snac::DebugDrawer::Level::debug).addBox(
            snac::DebugDrawer::Entry{
                .mPosition = {0.f, 0.f, 0.f},
                .mColor = math::hdr::gBlue<float>,
            },
            playerHitbox
        );

        mPills.each([&eatPillUpdate, &playerHitbox, &aLifeCycle]
                    (ent::Handle<ent::Entity> aHandle,
                     const component::GlobalPose & aPillGeo,
                     const component::Collision & aPillCol) 
        {
            Box_f pillHitbox = component::transformHitbox(aPillGeo.mPosition,
                                                          aPillCol.mHitbox);

            if (component::collideWithSat(pillHitbox, playerHitbox) && aLifeCycle.mInvulFrameCounter <= 0.f)
            {
                aHandle.get(eatPillUpdate)->erase();
                aLifeCycle.mScore += gPointPerPill;

            }
        });

        // TODO Should only happen on pill eating (collision),
        // but it would show the previous round score until 1st pill of next round is eaten (stunfest Q&D)
        // Update the text showing the score in the hud.
        // TODO code smell, this is defensive programming because sometimes we get there without the HUD
        // (when the round monitor removed the hud but the spawner did not populate it yet)
        // This is a great way to increase cyclomatic complexity
        if(aLifeCycle.mHud && aLifeCycle.mHud->isValid())
        {
            auto & playerHud = snac::getComponent<component::PlayerHud>(*aLifeCycle.mHud);
            snac::getComponent<component::Text>(playerHud.mScoreText)
                    .mString = std::to_string(aLifeCycle.mScore);
            snac::getComponent<component::Text>(playerHud.mRoundText)
                    .mString = std::to_string(aLifeCycle.mRoundsWon);
        }
    });

    mPills.each([](const component::GlobalPose & aPillPose, const component::Collision & aPillCol) {
        Box_f pillHitbox = component::transformHitbox(aPillPose.mPosition,
                                                      aPillCol.mHitbox);
        DBGDRAW(snac::gHitboxDrawer, snac::DebugDrawer::Level::debug)
            .addBox(
                snac::DebugDrawer::Entry{
                    .mPosition = {0.f, 0.f, 0.f},
                    .mColor = math::hdr::gBlue<float>,
                },
                pillHitbox);
    });
}


} // namespace system
} // namespace snacgame
} // namespace ad
