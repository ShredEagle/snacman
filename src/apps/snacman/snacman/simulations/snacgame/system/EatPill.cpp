#include "EatPill.h"
#include "snacman/simulations/snacgame/component/GlobalPose.h"

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
                         const component::PlayerSlot & aSlot,
                         component::Collision aPlayerCol,
                         component::PlayerLifeCycle & aPlayerData,
                         component::Text & aText) {
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

        mPills.each([&eatPillUpdate, &playerHitbox, &aPlayerData, &aSlot,
                     &aText](ent::Handle<ent::Entity> aHandle,
                             const component::GlobalPose & aPillGeo,
                             const component::Collision & aPillCol) {
            Box_f pillHitbox = component::transformHitbox(aPillGeo.mPosition,
                                                          aPillCol.mHitbox);

            if (component::collideWithSat(pillHitbox, playerHitbox))
            {
                aHandle.get(eatPillUpdate)->erase();
                aPlayerData.mPoints += gPointPerPill;
                std::ostringstream playerText;
                playerText << "P" << aSlot.mIndex + 1 << " "
                           << aPlayerData.mPoints;
                aText.mString = playerText.str();
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
