#include "GraphGui.h"

#include "TheGraph.h"

#include "runtime_reflect/DearImguiVisitor.h"

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
            GraphControls::gForwardKeys.begin(),
            GraphControls::gForwardKeys.end(),
            [](auto aKeyIt){return *aKeyIt;});

        imguiui::addCombo("Forward polygon mode",
            aGraph.mControls.mForwardPolygonMode,
            GraphControls::gPolygonModes.begin(),
            GraphControls::gPolygonModes.end(),
            [](auto aModeIt){return graphics::to_string(*aModeIt);});

        ImGui::SeparatorText("Shadow mapping");
        DearImguiVisitor v;
        r(v, aGraph.mShadowPass.mControls);

        ImGui::End();
    }
}


} // namespace ad::renderer