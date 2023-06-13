#include "EatPill.h"
#include "snacman/simulations/snacgame/component/PlayerGameData.h"
#include "snacman/simulations/snacgame/component/Text.h"

#include "../component/GlobalPose.h"
#include "../component/Collision.h"
#include "../component/Tags.h"
#include "../component/PlayerHud.h"
#include "../component/PlayerRoundData.h"
#include "../component/SceneNode.h"
#include "../component/PlayerGameData.h"
#include "../component/PlayerSlot.h"

#include "../GameParameters.h"
#include "../typedef.h"
#include "../GameContext.h"

#include <snacman/EntityUtilities.h>

#include <entity/EntityManager.h>
#include <entity/Entity.h>
#include <math/Homogeneous.h>
#include <math/Transformations.h>
#include <snacman/DebugDrawing.h>
#include <snacman/Profiling.h>


namespace ad {
namespace snacgame {
namespace system {

EatPill::EatPill(GameContext & aGameContext) :
    mPlayers{aGameContext.mWorld},
    mPills{aGameContext.mWorld},
    mHuds{aGameContext.mWorld}
{}


void EatPill::update()
{
    TIME_RECURRING_CLASSFUNC(Main);
    mPlayers.each([this](const component::GlobalPose & aPlayerGeo,
                         component::Collision aPlayerCol,
                         component::PlayerRoundData & aLifeCycle)
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
                aLifeCycle.mRoundScore += gPointPerPill;

            }
        });

        // TODO: Should only happen on pill eating (collision),
        // but it would show the previous round score until 1st pill of next round is eaten (stunfest Q&D)
        // Update the text showing the score in the hud.
        // TODO: code smell, this is defensive programming because sometimes we get there without the HUD
        // (when the round monitor removed the hud but the spawner did not populate it yet)
        // This is a great way to increase cyclomatic complexity
        const component::PlayerGameData & playerData = aLifeCycle.mSlotHandle.get()->get<component::PlayerGameData>();
        OptEntHandle hud = snac::getComponent<component::PlayerGameData>(aLifeCycle.mSlotHandle).mHud;
        if(hud && hud->isValid())
        {
            auto & playerHud = snac::getComponent<component::PlayerHud>(*hud);
            snac::getComponent<component::Text>(playerHud.mScoreText)
                    .mString = std::to_string(aLifeCycle.mRoundScore);
            snac::getComponent<component::Text>(playerHud.mRoundText)
                    .mString = std::to_string(playerData.mRoundsWon);
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
