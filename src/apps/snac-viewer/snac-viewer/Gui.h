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

    void draw();

    imguiui::ImguiUi mImguiUi;
    bool f = false;
};


inline void Gui::draw()
{
    {
        mImguiUi.newFrame();
        // We must make sure to match each newFrame() to a render(),
        Guard renderGuard{[this]()
        {
            this->mImguiUi.render();
        }};


        ImGui::Begin("Render Controls");
        ImGui::Checkbox("Dummy", &f);
        ImGui::End();
    }

    mImguiUi.renderBackend();
}


} // namespace snac
} // namespace ad
