#include "Geometry.h"
#include "GlobalPose.h"
#include "../InputConstants.h"
#include "PlayerMoveState.h"

#include "imgui.h"

namespace ad {
namespace snacgame {
namespace component {
void Geometry::drawUi() const
{
    int intPosX =
        static_cast<int>(mPosition.x() + 0.5f);
    int intPosY =
        static_cast<int>(mPosition.y() + 0.5f);
    float fracPosX = mPosition.x() - intPosX;
    float fracPosY = mPosition.y() - intPosY;
    ImGui::Text("Player pos: %f, %f", mPosition.x(),
                mPosition.y());
    ImGui::Text("Player integral part: %d, %d", intPosX, intPosY);
    ImGui::Text("Player frac part: %f, %f", fracPosX, fracPosY);
    ImGui::Text("Player orientation: %f, (%f, %f, %f)", mOrientation.w(),
                mOrientation.x(), mOrientation.y(), mOrientation.z());
    ImGui::Text("Player instance scaling: %f, %f %f", mInstanceScaling.width(),
                mInstanceScaling.height(), mInstanceScaling.depth());
}

void GlobalPose::drawUi() const
{
    ImGui::Text("Player global pos: %f, %f, %f", mPosition.x(),
                mPosition.y(), mPosition.z());
    ImGui::Text("Player global scaling: %f", mScaling);
    ImGui::Text("Player global orientation: %f, (%f, %f, %f)", mOrientation.w(),
                mOrientation.x(), mOrientation.y(), mOrientation.z());
    ImGui::Text("Player global instance scaling: %f, %f %f", mInstanceScaling.width(),
                mInstanceScaling.width(), mInstanceScaling.depth());
}

void PlayerMoveState::drawUi() const
{
    ImGui::Text("Current portal %d", mCurrentPortal);
    ImGui::Text("Dest portal %d", mDestinationPortal);
    ImGui::Text("Player MoveState:");
    if (mAllowedMove & gPlayerMoveFlagDown)
    {
        ImGui::SameLine();
        ImGui::Text("Down");
    }
    if (mAllowedMove & gPlayerMoveFlagUp)
    {
        ImGui::SameLine();
        ImGui::Text("Up");
    }
    if (mAllowedMove & gPlayerMoveFlagRight)
    {
        ImGui::SameLine();
        ImGui::Text("Right");
    }
    if (mAllowedMove & gPlayerMoveFlagLeft)
    {
        ImGui::SameLine();
        ImGui::Text("Left");
    }
}
} // namespace component
} // namespace snacgame
} // namespace ad
