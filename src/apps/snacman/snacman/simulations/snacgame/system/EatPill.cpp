#include "EatPill.h"

#include "entity/Entity.h"
#include "math/Homogeneous.h"
#include "snacman/Profiling.h"
#include "snacman/simulations/snacgame/SnacGame.h"

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

        float zCoord = static_cast<float>((int)aPlayerGeo.mLayer) * gCellSize * 0.1f;
        math::Vec<3,float> worldPos{
            aPlayerGeo.mPosition.x(),
            aPlayerGeo.mPosition.y(),
            zCoord,
        };

        auto playerHitbox = aPlayerCol.mHitbox;
        math::Position<4, float> transformedPos =  math::homogeneous::makePosition(playerHitbox.mPosition) * math::trans3d::translate(worldPos);
        playerHitbox.mPosition = transformedPos.xyz();
        
        mPills.each([&](ent::Handle<ent::Entity> aHandle,
                        const component::Geometry & aPillGeo,
                        const component::Collision & aPillCol) {
            float zCoord = static_cast<float>((int)aPillGeo.mLayer) * gCellSize * 0.1f;
            math::Vec<3,float> worldPos{
                aPillGeo.mPosition.x(),
                aPillGeo.mPosition.y(),
                zCoord,
            };

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
