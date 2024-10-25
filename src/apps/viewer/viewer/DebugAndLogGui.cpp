#include "DebugAndLogGui.h"


#include <imguiui/ImguiUi.h>
#include <imguiui/Widgets.h>


#include <utilities/ImguiUtilities.h>


namespace ad::renderer {
    
    
void DebugAndLogGui::present()
{
    if(imguiui::addCheckbox("Debug Draw", mShowDebugDraw))
    {
        ImGui::Begin("Debug Draw", &mShowDebugDraw);

        imguiui::addCheckbox("Main view", mRenderToMainView);
        imguiui::addCheckbox("Secondary view", mRenderToSecondaryView);

        utilities::imguiDebugDrawerLevelSection();

        ImGui::End();
    }

    if(imguiui::addCheckbox("Logging", mShowLog))
    {
        ImGui::Begin("Logging", &mShowLog);
        utilities::imguiLogLevelSelection();
        ImGui::End();
    }
}


} // namespace ad::renderer