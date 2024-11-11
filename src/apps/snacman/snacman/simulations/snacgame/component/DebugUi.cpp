#include "AllowedMovement.h"
#include "Controller.h"
#include "Geometry.h"
#include "GlobalPose.h"
#include "math/Curves/Bezier.h"
#include "math/Interpolation/ParameterAnimation.h"
#include "snacman/Profiling.h"
#include "snacman/simulations/snacgame/component/Text.h"

#include "../InputConstants.h"
#include "../LevelHelper.h"
#include "../typedef.h"

#include <cmath>
#include <imgui.h>
#include <implot.h>

namespace ad {
namespace snacgame {
namespace component {
void Geometry::drawUi() const
{
    Pos2_i intPos = getLevelPosition_i(mPosition.xy());
    float fracPosX = mPosition.x() - (float)intPos.x();
    float fracPosY = mPosition.y() - (float)intPos.y();
    ImGui::Text("Player pos: %f, %f", mPosition.x(), mPosition.y());
    ImGui::Text("Player integral part: %d, %d", intPos.x(), intPos.y());
    ImGui::Text("Player frac part: %f, %f", fracPosX, fracPosY);
    ImGui::Text("Player orientation: %f, (%f, %f, %f)", mOrientation.w(),
                mOrientation.x(), mOrientation.y(), mOrientation.z());
    ImGui::Text("Player instance scaling: %f, %f %f", mInstanceScaling.width(),
                mInstanceScaling.height(), mInstanceScaling.depth());
}

void GlobalPose::drawUi() const
{
    ImGui::Text("Player global pos: %f, %f, %f", mPosition.x(), mPosition.y(),
                mPosition.z());
    ImGui::Text("Player global scaling: %f", mScaling);
    ImGui::Text("Player global orientation: %f, (%f, %f, %f)", mOrientation.w(),
                mOrientation.x(), mOrientation.y(), mOrientation.z());
    ImGui::Text("Player global instance scaling: %f, %f %f",
                mInstanceScaling.width(), mInstanceScaling.width(),
                mInstanceScaling.depth());
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

int getIndexAroundPosition(math::Position<2, float> aPos, float * aPointsXs, float * aPointsYs, size_t aSize, float aRadius)
{
    for (int i = 0; i < (int)aSize; i++)
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

void drawBezierUi(ParameterAnimation<float,
                                     math::AnimationResult::FullRange,
                                     math::None,
                                     math::ease::Bezier> & aCurve)
{
    TIME_RECURRING_CLASSFUNC(Main);
    // TODO: (franz): maybe implot wasn't the best choice
    if (ImGui::CollapsingHeader("Change size"))
    {
        constexpr size_t numberOfSegments = 100;
        std::array<float, numberOfSegments> yValues;
        std::array<float, numberOfSegments> xValues;
        auto cubic_root_test = BEGIN_RECURRING(Main, "cubic_roots");
        float xStep = 1.f / (float) numberOfSegments;
        for (int i = 0; i < (int)numberOfSegments; i++)
        {
            float x = (float)i * xStep;
            xValues.at(i) = x;
            yValues.at(i) = aCurve.at(x);
        }
        END_RECURRING(cubic_root_test);

        auto draw_plot = BEGIN_RECURRING(Main, "draw_plot");
        ImPlot::BeginPlot("Plot", ImVec2(-1, 0),
                          ImPlotFlags_NoTitle | ImPlotFlags_NoLegend);
        ImPlot::SetupAxes(nullptr, nullptr);
        ImPlot::SetupAxesLimits(-0.1f, 1.1f, -0.1f, 1.1f, ImGuiCond_Always);
        ImPlot::PlotLine("Curve", xValues.data(), yValues.data(),
                         (int)yValues.size());
        END_RECURRING(draw_plot);

        auto knots = aCurve.mEaser.getKnots();
        bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        bool plotHovered = ImPlot::IsPlotHovered();
        ImPlotPoint mouse = ImPlot::GetPlotMousePos();
        ImVec2 imguiMouseScreenPos = ImGui::GetMousePos();
        math::Position<2, float> mouseScreenPos = {imguiMouseScreenPos.x,
                                                   imguiMouseScreenPos.y};
        math::Position<2, float> mousePos = {(float) mouse.x, (float) mouse.y};
        float yValue = aCurve.at((float)mouse.x);

        constexpr size_t maxOnCurve = math::ease::Bezier<float>::sMaxOnCurve;
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
        // At the end we need to extract the last on curve node because we skip
        // it during the loop (since it's the only node that is not the p1 of
        // any curve)
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

        nearestIndex = getIndexAroundPosition(mouseScreenPos, xsOn.data(),
                                              ysOn.data(), onSize, 10.f)
                       * 3;

        if (nearestIndex < 0 || ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
        {
            nearestIndex = getIndexAroundPosition(mouseScreenPos, xsOff.data(),
                                                  ysOff.data(), offSize, 10.f);
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
            ImPlot::PlotInfLines("Vertical", horiLine, 1,
                                 ImPlotInfLinesFlags_Horizontal);
            ImPlot::TagY(yValue, ImVec4(1, 1, 0, 1), "%.2f", yValue);
        }

        if (!mouseDown)
        {
            selectedIndex = -1;
        }

        if (plotHovered && selectedIndex == -1 && mouseClicked)
        {
            math::Position<2, float> pointOnLine{(float) mouse.x, yValue};
            if ((mousePos - pointOnLine).getNormSquared() < 0.005)
            {
                selectedIndex = (int)aCurve.mEaser.addPoint((float) mouse.x);
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_H) && nearestIndex != -1
            && nearestIndex % 3 == 0 && nearestIndex > 1
            && nearestIndex < (int)knots.size() - 1)
        {
            aCurve.mEaser.changePoint(
                nearestIndex - 1,
                {knots.at(nearestIndex - 1).x(), knots.at(nearestIndex).y()});
            aCurve.mEaser.changePoint(
                nearestIndex + 1,
                {knots.at(nearestIndex + 1).x(), knots.at(nearestIndex).y()});
        }

        if (ImGui::IsKeyPressed(ImGuiKey_X) && nearestIndex != -1
            && nearestIndex % 3 == 0 && nearestIndex > 1
            && nearestIndex < (int)knots.size() - 1)
        {
            aCurve.mEaser.removePoint(nearestIndex);
        }

        if (mouseDown && selectedIndex != -1)
        {
            if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
            {
                mousePos.y() = knots.at(selectedIndex).y();
            }

            if (!ImGui::IsKeyDown(ImGuiKey_LeftAlt) && selectedIndex > 1
                && selectedIndex < (int)knots.size() - 2)
            {
                switch (selectedIndex % 3)
                {
                case 1:
                {
                    math::Position<2, float> onCurvePos =
                        knots.at(selectedIndex - 1);
                    math::Vec<2, float> displacement = mousePos - onCurvePos;
                    aCurve.mEaser.changePoint(selectedIndex - 2,
                                              onCurvePos - displacement);
                    break;
                }
                case 2:
                {
                    math::Position<2, float> onCurvePos =
                        knots.at(selectedIndex + 1);
                    math::Vec<2, float> displacement = mousePos - onCurvePos;
                    aCurve.mEaser.changePoint(selectedIndex + 2,
                                              onCurvePos - displacement);
                    break;
                }
                }
            }
            aCurve.mEaser.changePoint(selectedIndex, mousePos);
        }

        ImPlot::SetNextMarkerStyle(ImPlotMarker_Diamond);
        ImPlot::PlotScatter("On curve", xsOn.data(), ysOn.data(), (int) onSize);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, IMPLOT_AUTO,
                                   ImVec4(0, 0, 0, 0), 1.f, ImVec4(1, 1, 0, 1));
        ImPlot::PlotScatter("Off curve", xsOff.data(), ysOff.data(),
                            (int) offSize);
        if (nearestIndex != -1)
        {
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, IMPLOT_AUTO,
                                       ImVec4(0, 1, 0, 1));
            ImPlot::PlotScatter("nearest", &knots.at(nearestIndex).x(),
                                &knots.at(nearestIndex).y(), 1);
        }
        for (int i = 0; i < (int)onSize * 2 - 2; i++)
        {
            ImPlot::PlotLine("Curve handle", xlines.at(i).data(),
                             ylines.at(i).data(), 2);
        }
        ImPlot::EndPlot();
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

void TextZoom::debugRender(const char * n)
{
    if (ImGui::TreeNode(n))
    {
        auto & paramAnim = mParameter;
        ImGui::SliderFloat("mass", &paramAnim.mEaser.mMass, 0.01f,  10.f);
        ImGui::SliderFloat("dampening", &paramAnim.mEaser.mDampening, 0.f, std::sqrt(4 * paramAnim.mEaser.mSpringStrength * paramAnim.mEaser.mMass));
        ImGui::SliderFloat("spring strength", &paramAnim.mEaser.mSpringStrength, 0.f, 10.f);
        ImGui::SliderFloat("initial velocity", &paramAnim.mEaser.mInitialVelocity, 0.f, 10.f);
        ImGui::Text("Start time %d", (int)snac::asSeconds(mStartTime.time_since_epoch()));
        if (ImPlot::BeginPlot("Plot", ImVec2(-1, 0),
                      ImPlotFlags_NoTitle | ImPlotFlags_NoLegend))
        {
            ImPlot::SetupAxes(nullptr, nullptr);
            ImPlot::SetupAxesLimits(-0.1f, 1.1f, -0.1f, 1.5f, ImGuiCond_Always);
            constexpr int resolution = 1000;
            std::array<float, resolution> xValues;
            std::array<float, resolution> yValues;
            int i = 0;
            for (float & x : xValues)
            {
                x = (float)i / (float)resolution;
                float y = paramAnim.at(x * paramAnim.getPeriod());
                yValues.at(i) = y;
                i++;
            }
            ImPlot::PlotLine("Curve", xValues.data(), yValues.data(),
                         resolution);
            ImPlot::EndPlot();
        }
        if (ImGui::Button("reset timer"))
        {
            mStartTime = ad::snac::Clock::now();
        }
        ImGui::TreePop();
    }
}
} // namespace component
} // namespace snacgame
} // namespace ad
