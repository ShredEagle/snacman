#include "EatPill.h"
#include "snacman/simulations/snacgame/component/GlobalPose.h"
#include "snacman/simulations/snacgame/component/PlayerHud.h"

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
                         component::PlayerLifeCycle & aPlayerLifeCycle)
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

        mPills.each([&eatPillUpdate, &playerHitbox, &aPlayerLifeCycle]
                    (ent::Handle<ent::Entity> aHandle,
                     const component::GlobalPose & aPillGeo,
                     const component::Collision & aPillCol) 
        {
            Box_f pillHitbox = component::transformHitbox(aPillGeo.mPosition,
                                                          aPillCol.mHitbox);

            if (component::collideWithSat(pillHitbox, playerHitbox))
            {
                aHandle.get(eatPillUpdate)->erase();
                aPlayerLifeCycle.mScore += gPointPerPill;
            }
        });
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
