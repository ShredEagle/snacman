#include "DebugDrawGui.h"


#include <imguiui/ImguiUi.h>
#include <imguiui/Widgets.h>


namespace ad::renderer {
    
    
void DebugDrawGui::present()
{
    if(imguiui::addCheckbox("Debug Draw", mShow))
    {
        ImGui::Begin("Debug Draw", nullptr);

        imguiui::addCheckbox("Main view", mRenderToMainView);
        imguiui::addCheckbox("Secondary view", mRenderToSecondaryView);

        ImGui::End();
    }
}


} // namespace ad::renderer