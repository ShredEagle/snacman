#pragma once

#include "math/Color.h"
#include "math/Quaternion.h"
#include "math/Spherical.h"
#include <math/Vector.h>

#include <algorithm>
#include <imgui.h>
#include <map>
#include <vector>
#include <string>
#include <type_traits>
#include <unordered_map>

void debugRender(const char * n, int & a);
void debugRender(const char * n, const int & a);
void debugRender(const char * n, bool & a);
void debugRender(const char * n, const bool & a);
void debugRender(const char * n, std::string & a);
void debugRender(const char * n, const std::string & a);
void debugRender(const char * n, float & a);
void debugRender(const char * n, const float & a);
void debugRender(const char * n, ad::math::Vec<3, float> & a);
void debugRender(const char * n, ad::math::Vec<2, float> & a);
void debugRender(const char * n, ad::math::Position<3, float> & a);
void debugRender(const char * n, ad::math::Position<2, float> & a);
void debugRender(const char * n, ad::math::Size<3, float> & a);
void debugRender(const char * n, ad::math::Size<2, float> & a);
void debugRender(const char * n, ad::math::Quaternion<float> & a);
void debugRender(const char * n, ad::math::Spherical<float> & a);
void debugRender(const char * n, ad::math::Rgb_base<float> & a);
void debugRender(const char * n, ad::math::Rgb_base<std::uint8_t> & a);
void debugRender(const char * n, ad::math::RgbAlpha_base<float> & a);
void debugRender(const char * n, ad::math::RgbAlpha_base<std::uint8_t> & a);

template<class T_key, class T_value, class... T_args, template<class...> class T_map_like>
requires (
    std::is_same_v<T_map_like<T_key, T_value, T_args...>, std::map<T_key, T_value>> || 
    std::is_same_v<T_map_like<T_key, T_value, T_args...>, std::unordered_map<T_key, T_value>> 
)
void debugRender(const char * n, T_map_like<T_key, T_value, T_args...>  & a)
{
    static T_key clickedNode;
    static bool set = false;
    if (ImGui::TreeNode(n))
    {
        float child_w = ImGui::GetContentRegionAvail().x;
        ImGui::BeginChild("rangechild", ImVec2(std::min(child_w, 100.f), 200.0f));
        for (auto & [key, value] : a)
        {
            //TODO(franz): This is a probleme with non conventional keys
             debugRender("", key);

            if (ImGui::IsItemClicked())
            {
                set = true;
                clickedNode = key;
            }
        }
        ImGui::EndChild();
        if (set)
        {
            ImGui::SameLine();
            debugRender("", a.at(clickedNode));
        }
        ImGui::TreePop();
    }
    else
    {
        //set = false;
    }
}


template<class T_value>
void debugRender(const char * n, std::vector<T_value>  & a)
{
    static int clickedNode = -1;
    if (ImGui::TreeNode(n))
    {
        float child_w = ImGui::GetContentRegionAvail().x;
        ImGui::BeginChild("rangechild", ImVec2(std::min(child_w, 100.f), 200.0f));
        for (std::size_t i = 0; i < a.size(); i++ )
        {
             ImGui::Text("%ld", i);

            if (ImGui::IsItemClicked())
            {
                clickedNode = (int)i;
            }
        }
        ImGui::EndChild();
        if (clickedNode > -1)
        {
            ImGui::SameLine();
            debugRender("", a.at(clickedNode));
        }
        ImGui::TreePop();
    }
    else
    {
        //clickedNode = -1;
    }
}

template<class T_value, std::size_t V_size>
void debugRender(const char * n, std::array<T_value, V_size>  & a)
{
    static int clickedNode = -1;
    if (ImGui::TreeNode(n))
    {
        float child_w = ImGui::GetContentRegionAvail().x;
        ImGui::BeginChild("rangechild", ImVec2(std::min(100.f, child_w), 200.0f));
        for (std::size_t i = 0; i < a.size(); i++ )
        {
             ImGui::Text("%ld", i);

            if (ImGui::IsItemClicked())
            {
                clickedNode = (int)i;
            }
        }
        ImGui::EndChild();
        if (clickedNode > -1)
        {
            ImGui::SameLine();
            debugRender("", a.at(clickedNode));
        }
        ImGui::TreePop();
    }
    else
    {
        //clickedNode = -1;
    }
}
