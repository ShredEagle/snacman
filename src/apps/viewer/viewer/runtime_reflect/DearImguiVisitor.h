#pragma once


#include <imgui.h>


namespace ad {


struct DearImguiVisitor
{};


//template <class T_value>
//void r(DearImguiVisitor & aV, T_value & aValue, const char * aName)
//{
//    ImGui::Text(aName);
//}

void r(DearImguiVisitor & aV, float  & aFloat, const char * aName)
{
    ImGui::InputFloat(aName, &aFloat);
}


void r(DearImguiVisitor & aV, math::Position<3, GLfloat> & aPos, const char * aName)
{
    ImGui::InputFloat3(aName, aPos.data());
}


void r(DearImguiVisitor & aV, math::Vec<3, GLfloat> & aVec, const char * aName)
{
    ImGui::InputFloat3(aName, aVec.data());
}


void r(DearImguiVisitor & aV, math::UnitVec<3, GLfloat> & aVec, const char * aName)
{
    ImGui::InputFloat3(aName, aVec.data());
    aVec.normalize();
}


void r(DearImguiVisitor & aV, math::hdr::Rgb<GLfloat> & aRgb, const char * aName)
{
    ImGui::ColorEdit3(aName, aRgb.data());
}


} // namespace ad