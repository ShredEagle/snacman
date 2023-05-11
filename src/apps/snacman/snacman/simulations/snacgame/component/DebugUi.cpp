#include "Geometry.h"
#include "GlobalPose.h"
#include "AllowedMovement.h"
#include "Controller.h"
#include "PlayerMoveState.h"

#include "../typedef.h"
#include "../InputConstants.h"
#include "../LevelHelper.h"

#include <imgui.h>

namespace ad {
namespace snacgame {
namespace component {
void Geometry::drawUi() const
{
    Pos2_i intPos = getLevelPosition_i(mPosition.xy());
    float fracPosX = mPosition.x() - intPos.x();
    float fracPosY = mPosition.y() - intPos.y();
    ImGui::Text("Player pos: %f, %f", mPosition.x(),
                mPosition.y());
    ImGui::Text("Player integral part: %d, %d", intPos.x(), intPos.y());
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
}

void AllowedMovement::drawUi() const
{
    ImGui::Text("AllowedMovement:");
    if (mAllowedMovement & gPlayerMoveFlagDown)
    {
        ImGui::SameLine();
        ImGui::Text("Down");
    }
    if (mAllowedMovement & gPlayerMoveFlagUp)
    {
        ImGui::SameLine();
        ImGui::Text("Up");
    }
    if (mAllowedMovement & gPlayerMoveFlagRight)
    {
        ImGui::SameLine();
        ImGui::Text("Right");
    }
    if (mAllowedMovement & gPlayerMoveFlagLeft)
    {
        ImGui::SameLine();
        ImGui::Text("Left");
    }
}

void Controller::drawUi() const
{
    ImGui::Text("Controller id %d", mControllerId);
    switch (mType)
    {
        case ControllerType::Keyboard:
            ImGui::Text("Keyboard");
            break;
        case ControllerType::Gamepad:
            ImGui::Text("Gamepad");
            break;
        case ControllerType::Dummy:
            ImGui::Text("Dummy");
            break;
    }
}
} // namespace component
} // namespace snacgame
} // namespace ad
