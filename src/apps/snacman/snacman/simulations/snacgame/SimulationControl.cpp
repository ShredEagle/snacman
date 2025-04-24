#include "SimulationControl.h"

#include <imgui.h>
#include <cstdio>
#include <algorithm>

namespace ad {
namespace snacgame {

constexpr float gMinRectWidth = 10.f;

constexpr ImU32 colorNoState = IM_COL32(50, 0, 200, 255);
constexpr ImU32 unselectedBorderColor = IM_COL32(233, 233, 233, 255);
constexpr ImU32 selectedBorderColor = IM_COL32(0, 233, 0, 255);
constexpr float rectHeight = 40.f;
constexpr int maxSpeedRatio = 64;
constexpr int minSpeedRatio = 1;
constexpr int speedRatioStep = 2;


void SimulationControl::drawSimulationUi(ent::EntityManager & aWorld, bool * open)
{
    ImGui::SetNextWindowSizeConstraints(
        ImVec2{gMinRectWidth * gNumberOfSavedStates, -1}, ImVec2{-1, -1});
    ImGui::Begin("Game state", open);
    if (drawVhsButton(VhsButtonType::FastRewind, "###fast rewind")
        && mSpeedRatio < maxSpeedRatio)
    {
        mSpeedRatio *= speedRatioStep;
    }
    ImGui::SameLine();
    if (mPlaying && drawVhsButton(VhsButtonType::Pause, "###pause"))
    {
        mPlaying = false;
    }
    if (!mPlaying && drawVhsButton(VhsButtonType::Play, "###play"))
    {
        mPlaying = true;
        mSelectedSaveState = -1;
    }
    ImGui::SameLine();
    mStep = false;
    if (drawVhsButton(VhsButtonType::Step, "###step"))
    {
        mStep = true;
    }
    ImGui::SameLine();
    if (drawVhsButton(VhsButtonType::FastForward, "###fast forward")
        && mSpeedRatio > minSpeedRatio)
    {
        mSpeedRatio /= speedRatioStep;
    }
    ImGui::SameLine();
    ImGui::Text("1/%ix", mSpeedRatio);
    if (ImGui::Button("Start saving states"))
    {
        mSaveGameState = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop saving states"))
    {
        mSaveGameState = false;
    }
    std::array<ImU32, gNumberOfSavedStates> colors{};
    for (std::size_t i = 0; i < gNumberOfSavedStates; ++i)
    {
        float tone = 200.f * (float) i / (float) gNumberOfSavedStates;
        colors.at(i) = IM_COL32(tone, tone, 0, 255);
    }
    ImGuiStyle & style = ImGui::GetStyle();
    style.AntiAliasedLines = false;

    ImDrawList * drawList = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float rectWidth = std::max(
        style.WindowPadding.x / gNumberOfSavedStates,
        gMinRectWidth);
    float x = p.x;
    float y = p.y;
    std::size_t currentIndex = 0;

    for (auto & saveState : mPreviousGameState)
    {
        if (currentIndex > 0)
        {
            ImGui::SameLine(0.f, 0.f);
        }

        char idBuf[128];
        std::snprintf(idBuf, 128, "%lu",
                      static_cast<unsigned long>(currentIndex));
        ImGui::PushID(idBuf);
        // We push an id because dummy don't have id and we need to
        // differentiate between them for the tooltip to work
        int forwardId = static_cast<int>(mSaveStateIndex - currentIndex);
        forwardId = forwardId >= 0 ? (int) gNumberOfSavedStates - forwardId - 1
                                   : -forwardId;
        ImU32 rectColor = saveState ? colors.at(forwardId) : colorNoState;
        ImVec2 rectPos(x, y);
        ImVec2 rectPosMax(x + rectWidth, y + rectHeight);
        ImU32 borderColor = currentIndex == (std::size_t) mSelectedSaveState
                                ? selectedBorderColor
                                : unselectedBorderColor;

        drawList->AddRectFilled(rectPos, rectPosMax, rectColor);
        drawList->AddRect(rectPos, rectPosMax, borderColor, 0, 0, 1);
        ImGui::Dummy(ImVec2(rectWidth, rectHeight));
        if (ImGui::IsItemHovered() && saveState)
        {
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                aWorld.restoreState(saveState->mState);
                mSelectedSaveState = static_cast<int>(currentIndex);
            }
            ImGui::BeginTooltip();
            ImGui::Text("Frame duration: %.3f",
                        static_cast<float>(saveState->mFrameDuration.count())
                            / 1'000.f);
            ImGui::Text(
                "Update delta: %.3f",
                static_cast<float>(saveState->mUpdateDelta.count()) / 1'000.f);
            ImGui::EndTooltip();
        }
        ImGui::PopID();
        x += rectWidth;
        currentIndex++;
    }
    style.AntiAliasedLines = true;
    ImGui::End();
}

bool drawVhsButton(VhsButtonType aButtonType, const char * id)
{
    float buttonEdgeSize = ImGui::GetFontSize() * 1.46f;
    float pauseBarWidth = buttonEdgeSize / 5.f;
    ImVec2 buttonSize(buttonEdgeSize, buttonEdgeSize);
    ImDrawList * drawList = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float x = p.x;
    float y = p.y;
    bool pressed = ImGui::Button(id, buttonSize);

    switch (aButtonType)
    {
    case VhsButtonType::Play:
        drawList->AddTriangleFilled(
            ImVec2(x + pauseBarWidth, y + pauseBarWidth),
            ImVec2(x + pauseBarWidth * 4, y + buttonEdgeSize / 2),
            ImVec2(x + pauseBarWidth, y + buttonEdgeSize - pauseBarWidth),
            IM_COL32(233, 233, 233, 255));
        break;
    case VhsButtonType::Pause:
        drawList->AddRectFilled(
            ImVec2(x + pauseBarWidth, y + pauseBarWidth),
            ImVec2(x + pauseBarWidth * 2, y + buttonEdgeSize - pauseBarWidth),
            IM_COL32(233, 233, 233, 255));
        drawList->AddRectFilled(
            ImVec2(x + pauseBarWidth * 3, y + pauseBarWidth),
            ImVec2(x + pauseBarWidth * 4, y + buttonEdgeSize - pauseBarWidth),
            IM_COL32(233, 233, 233, 255));
        break;
    case VhsButtonType::FastForward:
        drawList->AddTriangleFilled(
            ImVec2(x + pauseBarWidth, y + pauseBarWidth),
            ImVec2(x + buttonEdgeSize / 2, y + buttonEdgeSize / 2),
            ImVec2(x + pauseBarWidth, y + buttonEdgeSize - pauseBarWidth),
            IM_COL32(233, 233, 233, 255));
        drawList->AddTriangleFilled(
            ImVec2(x + buttonEdgeSize / 2, y + pauseBarWidth + 1.f),
            ImVec2(x + pauseBarWidth * 4, y + buttonEdgeSize / 2),
            ImVec2(x + buttonEdgeSize / 2, y + buttonEdgeSize - pauseBarWidth),
            IM_COL32(233, 233, 233, 255));
        break;
    case VhsButtonType::FastRewind:
        drawList->AddTriangleFilled(
            ImVec2(x + buttonEdgeSize / 2, y + pauseBarWidth),
            ImVec2(x + buttonEdgeSize / 2, y + buttonEdgeSize - pauseBarWidth),
            ImVec2(x + pauseBarWidth, y + buttonEdgeSize / 2),
            IM_COL32(233, 233, 233, 255));
        drawList->AddTriangleFilled(
            ImVec2(x + pauseBarWidth * 4, y + pauseBarWidth),
            ImVec2(x + pauseBarWidth * 4, y + buttonEdgeSize - pauseBarWidth),
            ImVec2(x + buttonEdgeSize / 2, y + buttonEdgeSize / 2),
            IM_COL32(233, 233, 233, 255));
        break;
    case VhsButtonType::Step:
        drawList->AddRectFilled(
            ImVec2(x + pauseBarWidth, y + pauseBarWidth),
            ImVec2(x + pauseBarWidth * 2, y + buttonEdgeSize - pauseBarWidth),
            IM_COL32(233, 233, 233, 255));
        drawList->AddTriangleFilled(
            ImVec2(x + pauseBarWidth * 3, y + pauseBarWidth),
            ImVec2(x + pauseBarWidth * 4, y + buttonEdgeSize / 2),
            ImVec2(x + pauseBarWidth * 3, y + buttonEdgeSize - pauseBarWidth),
            IM_COL32(233, 233, 233, 255));
        break;
    default:
        break;
    }

    return pressed;
}
} // namespace snacgame
} // namespace ad
