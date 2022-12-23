#include "IntegratePlayerMovement.h"

#include "snac-renderer/Cube.h"

#include "../component/Level.h"

namespace ad {
namespace snacgame {
namespace system {

constexpr float gBasePlayerSpeed = 10.f;

void IntegratePlayerMovement::update(float aDelta)
{
    ent::Phase nomutation;
    float cellSize = mLevel.get(nomutation)->get<component::Level>().mCellSize;
    mPlayer.each(
        [cellSize, aDelta](component::Geometry & aGeometry,
                           const component::PlayerMoveState & aMoveState) {
            if (aMoveState.mMoveState == gPlayerMoveFlagUp)
            {
                aGeometry.mSubGridPosition +=
                    math::Vec<2, float>{0.f, -1.f} * gBasePlayerSpeed * aDelta;
            }
            else if (aMoveState.mMoveState == gPlayerMoveFlagDown)
            {
                aGeometry.mSubGridPosition +=
                    math::Vec<2, float>{0.f, 1.f} * gBasePlayerSpeed * aDelta;
            }
            else if (aMoveState.mMoveState == gPlayerMoveFlagLeft)
            {
                aGeometry.mSubGridPosition +=
                    math::Vec<2, float>{1.f, 0.f} * gBasePlayerSpeed * aDelta;
            }
            else if (aMoveState.mMoveState == gPlayerMoveFlagRight)
            {
                aGeometry.mSubGridPosition +=
                    math::Vec<2, float>{-1.f, 0.f} * gBasePlayerSpeed * aDelta;
            }

            if (aGeometry.mSubGridPosition.y() > cellSize / 2)
            {
                aGeometry.mGridPosition.y()++;
                aGeometry.mSubGridPosition.y() -= snac::gCubeSize;
                aGeometry.mSubGridPosition.x() = 0.f;
            }
            else if (aGeometry.mSubGridPosition.y() < -cellSize / 2)
            {
                aGeometry.mGridPosition.y()--;
                aGeometry.mSubGridPosition.y() += snac::gCubeSize;
                aGeometry.mSubGridPosition.x() = 0.f;
            }
            else if (aGeometry.mSubGridPosition.x() > cellSize / 2)
            {
                aGeometry.mGridPosition.x()++;
                aGeometry.mSubGridPosition.x() -= snac::gCubeSize;
                aGeometry.mSubGridPosition.y() = 0.f;
            }
            else if (aGeometry.mSubGridPosition.x() < -cellSize / 2)
            {
                aGeometry.mGridPosition.x()--;
                aGeometry.mSubGridPosition.x() += snac::gCubeSize;
                aGeometry.mSubGridPosition.y() = 0.f;
            }
        });
}

} // namespace system
} // namespace snacgame
} // namespace ad
