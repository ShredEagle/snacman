#include "AllowedMovement.h"
#include "Controller.h"
#include "Geometry.h"
#include "GlobalPose.h"
#include "ChangeSize.h"
#include "math/Interpolation/ParameterAnimation.h"

#include "../InputConstants.h"
#include "../LevelHelper.h"
#include "../typedef.h"

#include <snacman/Profiling.h>

#include <implot.h>

#include <imgui.h>

namespace ad {
namespace snacgame {
namespace component {
void Geometry::drawUi()
{
    if (ImGui::CollapsingHeader("Geometry"))
    {
        Pos2_i intPos = getLevelPosition_i(mPosition.xy());
        float fracPosX = mPosition.x() - (float)intPos.x();
        float fracPosY = mPosition.y() - (float)intPos.y();
        ImGui::InputFloat3("Pos", mPosition.data());
        ImGui::Text("Integral part: %d, %d", intPos.x(), intPos.y());
        ImGui::Text("Fractional part: %f, %f", fracPosX, fracPosY);
        ImGui::InputFloat4("Orientation: %f, (%f, %f, %f)", mOrientation.asVec().data());
        ImGui::InputFloat3("Instance scaling: %f, %f %f", mInstanceScaling.data());
    }
}

void GlobalPose::drawUi() const
{
    if (ImGui::CollapsingHeader("GlobalPose"))
    {
        ImGui::Text("pos: %f, %f, %f", mPosition.x(),
                    mPosition.y(), mPosition.z());
        ImGui::Text("scaling: %f", mScaling);
        ImGui::Text("orientation: %f, (%f, %f, %f)",
                    mOrientation.w(), mOrientation.x(), mOrientation.y(),
                    mOrientation.z());
        ImGui::Text("instance scaling: %f, %f %f",
                    mInstanceScaling.width(), mInstanceScaling.width(),
                    mInstanceScaling.depth());
    }
}

void AllowedMovement::drawUi() const
{
    if (ImGui::CollapsingHeader("AllowedMovement"))
    {
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
}

int getIndexAroundPosition(math::Position<2, float> aPos, float * aPointsXs, float * aPointsYs, size_t aSize, float aRadius)
{
    for (int i = 0; i < aSize; i++)
    {
        ImVec2 point = ImPlot::PlotToPixels(ImPlotPoint(aPointsXs[i], aPointsYs[i]));
        math::Position<2, float> pointPos{point.x, point.y};
        if ((aPos - pointPos).getNormSquared() < (aRadius * aRadius))
        {
            return i;
        }
    }
    return -1;
}

void ChangeSize::drawUi()
{
    TIME_RECURRING_CLASSFUNC(Main);
    // TODO: (franz): maybe implot wasn't the best choice
    if (ImGui::CollapsingHeader("Change size"))
    {
        constexpr size_t numberOfSegments = 100;
        std::array<float, numberOfSegments> yValues;
        std::array<float, numberOfSegments> xValues;
        BEGIN_RECURRING(Main, "cubic_roots", cubic_root_test);
        float xStep = 1.f / (float)numberOfSegments;
        for (int i = 0; i < numberOfSegments; i++)
        {
            float x = i * xStep;
            xValues.at(i) = x;
            yValues.at(i) = mCurve.at(x);
        }
        END_RECURRING(cubic_root_test);

        BEGIN_RECURRING(Main, "draw_plot", draw_plot);
        ImPlot::BeginPlot("Plot", ImVec2(-1, 0), ImPlotFlags_NoTitle | ImPlotFlags_NoLegend);
        ImPlot::SetupAxes(nullptr, nullptr);
        ImPlot::SetupAxesLimits(-0.1f, 1.1f, -0.1f, 1.1f, ImGuiCond_Always);
        ImPlot::PlotLine("Curve", xValues.data(), yValues.data(), yValues.size());
        END_RECURRING(draw_plot);

        auto knots = mCurve.mEaser.getKnots();
        bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        bool plotHovered = ImPlot::IsPlotHovered();
        ImPlotPoint mouse = ImPlot::GetPlotMousePos();
        ImVec2 imguiMouseScreenPos = ImGui::GetMousePos();
        math::Position<2, float> mouseScreenPos = {imguiMouseScreenPos.x, imguiMouseScreenPos.y};
        math::Position<2, float> mousePos = {(float)mouse.x, (float)mouse.y};
        float yValue = mCurve.at(mouse.x);

        constexpr size_t maxOnCurve = snac::detail::Bezier<float>::sMaxOnCurve; 
        constexpr size_t maxOffCurve = (maxOnCurve - 1) * 2; 
        std::array<float, maxOnCurve> xsOn;
        std::array<float, maxOffCurve> ysOn;
        const size_t onSize = (knots.size() + 2) / 3;
        std::array<float, maxOffCurve> xsOff;
        std::array<float, maxOffCurve> ysOff;
        const size_t offSize = (onSize - 1) * 2;
        std::array<std::array<float, 2>, maxOffCurve> xlines;
        std::array<std::array<float, 2>, maxOffCurve> ylines;
        static int nearestIndex = -1;
        static int selectedIndex = -1;

        // Knots is of this form :
        // p1, p2, p3, p4 and p1, p2, p3, p4 and p1, p2, p3...
        // So we extract the value of p1, p2, p3 for one instance of i
        // At the end we need to extract the last on curve node because we skip it
        // during the loop (since it's the only node that is not the p1 of any
        // curve)
        for (size_t i = 0; i < knots.size() / 3; i++)
        {
            xsOn.at(i) = knots.at(i * 3).x();
            ysOn.at(i) = knots.at(i * 3).y();

            xsOff.at(i * 2) = knots.at(i * 3 + 1).x();
            ysOff.at(i * 2) = knots.at(i * 3 + 1).y();

            xsOff.at(i * 2 + 1) = knots.at(i * 3 + 2).x();
            ysOff.at(i * 2 + 1) = knots.at(i * 3 + 2).y();


            xlines.at(i * 2).at(0) = xsOn.at(i);
            ylines.at(i * 2).at(0) = ysOn.at(i);
            xlines.at(i * 2).at(1) = xsOff.at(i * 2);
            ylines.at(i * 2).at(1) = ysOff.at(i * 2);

            xlines.at(i * 2 + 1).at(0) = xsOff.at(i * 2 + 1);
            ylines.at(i * 2 + 1).at(0) = ysOff.at(i * 2 + 1);
            xlines.at(i * 2 + 1).at(1) = knots.at(i * 3 + 3).x(); 
            ylines.at(i * 2 + 1).at(1) = knots.at(i * 3 + 3).y(); 
        }

        // Puts last on point in the data
        xsOn.at(onSize - 1) = knots.back().x();
        ysOn.at(onSize - 1) = knots.back().y();

        nearestIndex = getIndexAroundPosition(mouseScreenPos, xsOn.data(), ysOn.data(), onSize, 10.f) * 3;

        if (nearestIndex < 0 || ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
        {
            nearestIndex = getIndexAroundPosition(mouseScreenPos, xsOff.data(), ysOff.data(), offSize, 10.f);
            nearestIndex = nearestIndex / 2 * 3 + 1 + nearestIndex % 2;
            nearestIndex = nearestIndex == 0 ? -1 : nearestIndex;
        }

        if (mouseClicked)
        {
            selectedIndex = nearestIndex;
        }

        if (plotHovered)
        {
            ImPlotPoint mouse = ImPlot::GetPlotMousePos();
            double mouseLine[] = {mouse.x};
            ImPlot::PlotInfLines("Vertical", mouseLine, 1);
            ImPlot::TagX(mouse.x, ImVec4(1, 1, 0, 1), "%.2f", mouse.x);

            float horiLine[] = {yValue};
            ImPlot::PlotInfLines("Vertical", horiLine, 1, ImPlotInfLinesFlags_Horizontal);
            ImPlot::TagY(yValue, ImVec4(1, 1, 0, 1), "%.2f", yValue);

        }

        if (!mouseDown)
        {
            selectedIndex = -1;
        }

        if (plotHovered && selectedIndex == -1 && mouseClicked)
        {
            math::Position<2, float> pointOnLine{(float)mouse.x, yValue};
            if ((mousePos - pointOnLine).getNormSquared() < 0.005)
            {
                selectedIndex = mCurve.mEaser.addPoint((float)mouse.x);
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_H) && nearestIndex != -1 && nearestIndex % 3 == 0 && nearestIndex > 1 && nearestIndex < knots.size() - 1)
        {
            mCurve.mEaser.changePoint(nearestIndex - 1, {knots.at(nearestIndex - 1).x(), knots.at(nearestIndex).y()});
            mCurve.mEaser.changePoint(nearestIndex + 1, {knots.at(nearestIndex + 1).x(), knots.at(nearestIndex).y()});
        }

        if (ImGui::IsKeyPressed(ImGuiKey_X) && nearestIndex != -1 && nearestIndex % 3 == 0 && nearestIndex > 1 && nearestIndex < knots.size() - 1)
        {
            mCurve.mEaser.removePoint(nearestIndex);
        }

        if (mouseDown && selectedIndex != -1)
        {
            if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
            {
                mousePos.y() = knots.at(selectedIndex).y();
            }

            if (!ImGui::IsKeyDown(ImGuiKey_LeftAlt) && selectedIndex > 1 && selectedIndex < knots.size() - 2)
            {
                switch (selectedIndex % 3)
                {
                    case 1:
                    {
                        math::Position<2, float> onCurvePos = knots.at(selectedIndex - 1);
                        math::Vec<2, float> displacement = mousePos - onCurvePos;
                        mCurve.mEaser.changePoint(selectedIndex - 2, onCurvePos - displacement);
                        break;
                    }
                    case 2:
                    {
                        math::Position<2, float> onCurvePos = knots.at(selectedIndex + 1);
                        math::Vec<2, float> displacement = mousePos - onCurvePos;
                        mCurve.mEaser.changePoint(selectedIndex + 2, onCurvePos - displacement);
                        break;
                    }
                }
            }
            mCurve.mEaser.changePoint(selectedIndex, mousePos);
        }

        ImPlot::SetNextMarkerStyle(ImPlotMarker_Diamond);
        ImPlot::PlotScatter("On curve", xsOn.data(), ysOn.data(), (int)onSize);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, IMPLOT_AUTO, ImVec4(0, 0, 0, 0), 1.f, ImVec4(1, 1, 0, 1));
        ImPlot::PlotScatter("Off curve", xsOff.data(), ysOff.data(), (int)offSize);
        if (nearestIndex != -1)
        {
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, IMPLOT_AUTO, ImVec4(0, 1, 0, 1));
            ImPlot::PlotScatter("nearest", &knots.at(nearestIndex).x(), &knots.at(nearestIndex).y(), 1);
        }
        for (int i = 0; i < onSize * 2 - 2; i++)
        {
            ImPlot::PlotLine("Curve handle", xlines.at(i).data(), ylines.at(i).data(), 2);
        }
        ImPlot::EndPlot();
    }
}

void Controller::drawUi() const
{
    if (ImGui::CollapsingHeader("Controller"))
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
}
} // namespace component
} // namespace snacgame
} // namespace ad
