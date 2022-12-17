#include "IntegratePlayerMovement.h"

#include "../component/Level.h"

namespace ad {
namespace snacgame {
namespace system {

float gBasePlayerSpeed = 10.f;

void IntegratePlayerMovement::update(float aDelta)
{
    ent::Phase nomutation;
    float cellSize = mLevel.get(nomutation)->get<component::Level>().mCellSize;
    mPlayer.each([cellSize, aDelta](component::Geometry & aGeometry,
                          const component::PlayerMoveState & aMoveState) {
        if (aMoveState.mMoveState == gPlayerMoveUp)
        {
            aGeometry.mSubGridPosition +=
                math::Vec<2, float>{0.f, -1.f} * gBasePlayerSpeed * aDelta;
        }
        else if (aMoveState.mMoveState == gPlayerMoveDown)
        {
            aGeometry.mSubGridPosition +=
                math::Vec<2, float>{0.f, 1.f} * gBasePlayerSpeed * aDelta;
        }
        else if (aMoveState.mMoveState == gPlayerMoveLeft)
        {
            aGeometry.mSubGridPosition +=
                math::Vec<2, float>{1.f, 0.f} * gBasePlayerSpeed * aDelta;
        }
        else if (aMoveState.mMoveState == gPlayerMoveRight)
        {
            aGeometry.mSubGridPosition +=
                math::Vec<2, float>{-1.f, 0.f} * gBasePlayerSpeed * aDelta;
        }

        if (aGeometry.mSubGridPosition.y() > cellSize / 2)
        {
            aGeometry.mGridPosition.y() += 1;
            aGeometry.mSubGridPosition.y() -= 2.f;
            aGeometry.mSubGridPosition.x() = 0.f;
        }
        else if (aGeometry.mSubGridPosition.y() < -cellSize / 2)
        {
            aGeometry.mGridPosition.y() -= 1;
            aGeometry.mSubGridPosition.y() += 2.f;
            aGeometry.mSubGridPosition.x() = 0.f;
        }
        else if (aGeometry.mSubGridPosition.x() > cellSize / 2)
        {
            aGeometry.mGridPosition.x() += 1;
            aGeometry.mSubGridPosition.x() -= 2.f;
            aGeometry.mSubGridPosition.y() = 0.f;
        }
        else if (aGeometry.mSubGridPosition.x() < -cellSize / 2)
        {
            aGeometry.mGridPosition.x() -= 1;
            aGeometry.mSubGridPosition.x() += 2.f;
            aGeometry.mSubGridPosition.y() = 0.f;
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
