#include "SimulationControl.h"
#include "GameContext.h"
#include "SnacGame.h"

#include <imgui.h>
#include <imguiui/ImguiUi.h>

namespace ad {
namespace snacgame {

constexpr float gMinRectWidth = 10.f;

void SimulationControl::drawSimulationUi(ent::EntityManager & aWorld)
{
    ImGui::SetNextWindowSizeConstraints(ImVec2{gMinRectWidth * gNumberOfSavedStates, -1}, ImVec2{-1,-1});
    ImGui::Begin("Game state");
    if (drawVhsButton(VhsButtonType::FastRewind, "###fast rewind")
        && mSpeedFactor < 64.f)
    {
        mSpeedFactor *= 2.f;
    }
    ImGui::SameLine();
    if (mPlaying
        && drawVhsButton(VhsButtonType::Pause, "###pause"))
    {
        mPlaying = false;
    }
    if (!mPlaying
        && drawVhsButton(VhsButtonType::Play, "###play"))
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
        && mSpeedFactor >= 2.f)
    {
        mSpeedFactor /= 2.f;
    }
    ImGui::SameLine();
    ImGui::Text("1/%.0fx", mSpeedFactor);
    if (ImGui::Button("Start saving states"))
    {
        mSaveGameState = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop saving states"))
    {
        mSaveGameState = false;
    }
    std::array<ImU32, gNumberOfSavedStates> colors;
    for (std::size_t i = 0; i < gNumberOfSavedStates; ++i)
    {
        float tone = 200.f * (float)i / (float)gNumberOfSavedStates;
        colors.at(i) = IM_COL32(tone, tone, 0, 255);
    }
    ImGuiStyle & style = ImGui::GetStyle();
    style.AntiAliasedLines = false;

    ImU32 colorNoState = IM_COL32(50, 0, 200, 255);
    ImU32 borderColor = IM_COL32(233, 233, 233, 255);
    ImU32 selectedBorderColor = IM_COL32(0, 233, 0, 255);
    ImDrawList * drawList = ImGui::GetWindowDrawList();
    float windowWidth = ImGui::GetWindowWidth();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float rectWidth = std::max(
        (windowWidth - ImGui::GetCursorPosX() * 2) / gNumberOfSavedStates,
        gMinRectWidth);
    float x = p.x;
    float y = p.y;
    std::size_t currentId = 0;
    std::size_t activeId = mSaveStateIndex;
    float rectHeight = 40.f;

    for (auto & saveState : mPreviousGameState)
    {
        if (currentId > 0)
        {
            ImGui::SameLine(0.f, 0.f);
        }

        char idBuf[128];
        std::snprintf(idBuf, 128, "%lu", static_cast<unsigned long>(currentId));
        ImGui::PushID(idBuf);
        // We push an id because dummy don't have id and we need to
        // differentiate between them for the tooltip to work
        int forwardId = static_cast<int>(activeId) - static_cast<int>(currentId);

        drawList->AddRectFilled(
            ImVec2(x, y), ImVec2(x + rectWidth, y + rectHeight),
            saveState ? colors.at(forwardId >= 0
                                      ? gNumberOfSavedStates - 1
                                            - forwardId
                                      : - forwardId)
                      : colorNoState);
        drawList->AddRect(
            ImVec2(x, y), ImVec2(x + rectWidth, y + rectHeight),
            currentId == static_cast<std::size_t>(mSelectedSaveState) ? selectedBorderColor : borderColor, 0, ImDrawFlags_None, 1);
        ImGui::Dummy(ImVec2(rectWidth, rectHeight));
        if (ImGui::IsItemHovered() && saveState)
        {
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                aWorld.restoreState(saveState->mState);
                mSelectedSaveState = static_cast<int>(currentId);
            }
            ImGui::BeginTooltip();
            ImGui::Text("Frame duration: %.3f",
                        static_cast<float>(saveState->mFrameDuration.count())
                                           / 1'000.f);
            ImGui::Text("Update delta: %.3f",
                        static_cast<float>(saveState->mUpdateDelta.count())
                                           / 1'000.f);
            ImGui::EndTooltip();
        }
        ImGui::PopID();
        x += rectWidth;
        currentId++;
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
}
}
