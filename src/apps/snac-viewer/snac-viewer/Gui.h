#pragma once


#include <imguiui/ImguiUi.h>


namespace ad {
namespace snac {


struct Gui
{
public:
    Gui(const graphics::ApplicationGlfw & aApplication) :
        mImguiUi{aApplication}
    {}

    Guard startUI();
    void render();

    imguiui::ImguiUi mImguiUi;
};


inline Guard Gui::startUI()
{
    mImguiUi.newFrame();
    // We must make sure to match each newFrame() to a render(),
    Guard renderGuard{[this]()
    {
        this->mImguiUi.render();
    }};


    //ImGui::Begin("Generic Controls");
    //ImGui::End();

    return renderGuard;
}


inline void Gui::render()
{
    mImguiUi.renderBackend();
}


} // namespace snac
} // namespace ad
