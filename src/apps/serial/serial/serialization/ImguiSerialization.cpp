#include "ImguiSerialization.h"
#include <imgui.h>

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

void debugRender(const char * n, const std::string & a)
{
    ImGui::Text("%s", a.c_str());
}

void debugRender(const char * n, float & a)
{
    ImGui::InputFloat(n, &a);
}

