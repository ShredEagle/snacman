#include "GraphGui.h"

#include "TheGraph.h"

#include <imguiui/ImguiUi.h>
#include <imguiui/Widgets.h>
#include <imguiui/Widgets-impl.h>


namespace ad::renderer {


void GraphGui::present(TheGraph & aGraph)
{
    if(imguiui::addCheckbox("Render Graph", mShowGraph))
    {
        ImGui::Begin("Render Graph", nullptr);

        imguiui::addCombo("Forward pass",
            aGraph.mControls.mForwardPassKey,
            TheGraph::Controls::gForwardKeys.begin(),
            TheGraph::Controls::gForwardKeys.end(),
            [](auto aKeyIt){return *aKeyIt;});

        ImGui::End();
    }
}


} // namespace ad::renderer