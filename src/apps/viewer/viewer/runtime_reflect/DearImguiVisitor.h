#pragma once

#include "ReflectHelpers.h"

#include <imgui.h>


namespace ad {


struct DearImguiVisitor
{};


// Catch-all
template <class T_value>
void give(DearImguiVisitor & aV, T_value & aValue, const char * aName)
{
    ImGui::Text(aName);
    r(aV, aValue);
}


template <class T, std::size_t Extent>
void give(DearImguiVisitor & aV, const std::span<T, Extent> & aSpan, const char * aName)
{
    ImGui::Indent();
    for(std::size_t idx = 0; idx != aSpan.size(); ++idx)
    {
        std::string label = aName + (" #" + std::to_string(idx));
        ImGui::SeparatorText(label.c_str());
        // We have to push an explicit ID on the stack, to distinguish below widgets.
        ImGui::PushID(label.c_str());
        r(aV, aSpan[idx]);
        ImGui::PopID();
    }
    ImGui::Unindent();
}


template <std::integral T>
void give(DearImguiVisitor & aV, const Clamped<T> & aClamped, const char * aName)
{
    ImGui::InputScalar(aName, ImGuiDataType_U32, &aClamped.mValue);
    aClamped.mValue = std::clamp(aClamped.mValue, aClamped.mMin, aClamped.mMax);
}


void give(DearImguiVisitor & aV, float  & aFloat, const char * aName)
{
    ImGui::InputFloat(aName, &aFloat);
}


void give(DearImguiVisitor & aV, math::Position<3, GLfloat> & aPos, const char * aName)
{
    ImGui::InputFloat3(aName, aPos.data());
}


void give(DearImguiVisitor & aV, math::Vec<3, GLfloat> & aVec, const char * aName)
{
    ImGui::InputFloat3(aName, aVec.data());
}


void give(DearImguiVisitor & aV, math::UnitVec<3, GLfloat> & aVec, const char * aName)
{
    ImGui::InputFloat3(aName, aVec.data());
    aVec.normalize();
}


void give(DearImguiVisitor & aV, math::hdr::Rgb<GLfloat> & aRgb, const char * aName)
{
    ImGui::ColorEdit3(aName, aRgb.data());
}


} // namespace ad