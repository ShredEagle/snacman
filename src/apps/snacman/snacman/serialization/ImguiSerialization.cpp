#include "ImguiSerialization.h"
#include <imgui.h>
#include "snacman/Timing.h"
#include "snacman/simulations/snacgame/component/Text.h"

void debugRender(const char * n, int & a)
{
    ImGui::InputInt(n, &a);
}

void debugRender(const char * n, bool & a)
{
    ImGui::Checkbox(n, &a);
}

// TODO(franz): check imgui_stdlib on github to make a proper std::string
// render
void debugRender(const char * n, std::string & a)
{
    //ImGui::InputText(n, a.data(), a.size());
}

void debugRender(const char * n, float & a)
{
    ImGui::InputFloat(n, &a);
}

void debugRender(const char * n, const std::string & a)
{
    ImGui::Text("%s", a.c_str());
}

void debugRender(const char * n, const int & a)
{
    ImGui::Text("%d", a);
}

void debugRender(const char * n, const float & a)
{
    ImGui::Text("%f", a);
}


void debugRender(const char * n, const bool & a)
{
    ImGui::Text("%s", a ? "true" : "false");
}

void debugRender(const char * n, ad::math::Vec<3, float> & a)
{
    ImGui::InputFloat3(n, &a.at(0));
}

void debugRender(const char * n, ad::math::Vec<2, float> & a)
{
    ImGui::InputFloat2(n, &a.at(0));
}

void debugRender(const char * n, ad::math::Position<3, float> & a)
{
    ImGui::InputFloat3(n, &a.at(0));
}

void debugRender(const char * n, ad::math::Position<2, float> & a)
{
    ImGui::InputFloat2(n, &a.at(0));
}

void debugRender(const char * n, ad::math::Size<3, float> & a)
{
    ImGui::InputFloat3(n, &a.at(0));
}

void debugRender(const char * n, ad::math::Size<2, float> & a)
{
    ImGui::InputFloat2(n, &a.at(0));
}

void debugRender(const char * n, ad::math::Quaternion<float> & a)
{
    ImGui::SliderFloat4(n, &a.x(), -1.f, 1.f);
}

void debugRender(const char * n, ad::math::Spherical<float> & a)
{
    ImGui::InputFloat("radius", &a.radius());
    ImGui::InputFloat("polar", &a.polar().data());
    ImGui::InputFloat("azimuthal", &a.azimuthal().data());
}

void debugRender(const char * n, ad::math::Rgb_base<float> & a)
{
    ImGui::ColorEdit3(n, &a.at(0));
}

void debugRender(const char * n, ad::math::Rgb_base<std::uint8_t> & a)
{
    ImGui::ColorEdit3(n, (float*)&a.at(0), ImGuiColorEditFlags_Uint8);
}

void debugRender(const char * n, ad::math::RgbAlpha_base<float> & a)
{
    ImGui::ColorEdit4(n, &a.at(0));
}

void debugRender(const char * n, ad::math::RgbAlpha_base<std::uint8_t> & a)
{
    ImGui::ColorEdit4(n, (float*)&a.at(0), ImGuiColorEditFlags_Uint8);
}

