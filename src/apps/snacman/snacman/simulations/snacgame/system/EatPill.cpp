#include "EatPill.h"
#include "../GameParameters.h"
#include "../typedef.h"

#include "entity/Entity.h"
#include "math/Homogeneous.h"
#include "snacman/Profiling.h"
#include "snacman/simulations/snacgame/component/SceneNode.h"

#include <math/Transformations.h>

namespace ad {
namespace snacgame {
namespace system {
void EatPill::update()
{
    TIME_RECURRING_CLASSFUNC(Main);
    mPlayers.each([this](component::Geometry & aPlayerGeo,
                      const component::PlayerSlot & aSlot,
                      component::Collision aPlayerCol,
                      component::PlayerLifeCycle & aPlayerData,
                      component::Text & aText) {

        ent::Phase eatPillUpdate;
        Box_f playerHitbox = component::transformHitbox(aPlayerGeo.mPosition, aPlayerCol.mHitbox);
        
        mPills.each([&eatPillUpdate, &playerHitbox, &aPlayerData, &aSlot, &aText](ent::Handle<ent::Entity> aHandle,
                        const component::Geometry & aPillGeo,
                        const component::Collision & aPillCol) {
            Box_f pillHitbox = component::transformHitbox(aPillGeo.mPosition, aPillCol.mHitbox);

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
}
} // namespace system
} // namespace snacgame
} // namespace ad
