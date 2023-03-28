#include "EatPill.h"
#include "../GameParameters.h"

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
    ent::Phase eatPillUpdate;
    mPlayers.each([&](component::Geometry & aPlayerGeo,
                      const component::PlayerSlot & aSlot,
                      component::Collision aPlayerCol,
                      component::PlayerLifeCycle & aPlayerData,
                      component::Text & aText) {

        const math::Vec<3,float> & worldPos = aPlayerGeo.mPosition.as<math::Vec>();
        auto playerHitbox = aPlayerCol.mHitbox;
        math::Position<4, float> transformedPos =  math::homogeneous::makePosition(playerHitbox.mPosition) * math::trans3d::translate(worldPos);
        playerHitbox.mPosition = transformedPos.xyz();
        
        mPills.each([&](ent::Handle<ent::Entity> aHandle,
                        const component::Geometry & aPillGeo,
                        const component::Collision & aPillCol) {
            const math::Vec<3,float> & worldPos = aPillGeo.mPosition.as<math::Vec>();
            auto pillHitbox = aPillCol.mHitbox;
            math::Position<4, float> transformedPos =  math::homogeneous::makePosition(pillHitbox.mPosition) * math::trans3d::translate(worldPos);
            pillHitbox.mPosition = transformedPos.xyz();

            if (pillHitbox.xMin() <= playerHitbox.xMax()
                && pillHitbox.xMax() >= playerHitbox.xMin()
                && pillHitbox.yMin() <= playerHitbox.yMax()
                && pillHitbox.yMax() >= playerHitbox.yMin()
                && pillHitbox.zMin() <= playerHitbox.zMax()
                && pillHitbox.zMax() >= playerHitbox.zMin())
            {
                aHandle.get(eatPillUpdate)->erase();
                aPlayerData.mPoints += 10;
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
