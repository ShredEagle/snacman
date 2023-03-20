#pragma once


#include <imguiui/ImguiUi.h>

#include <spdlog/spdlog.h>


namespace ad {
namespace snac {

// TODO Those utilities should be more generic, and live in some shared repo.
void imguiLogLevelSelection(bool * open = nullptr)
{
    ImGui::Begin("Logging", open);
    {
        static const char* items[] = {
            "trace",
            "debug",
            "info",
            "warn",
            "err",
            "critical",
            "off",
        };

        spdlog::apply_all([&](std::shared_ptr<spdlog::logger> logger)
            {
                // I suspect this is the default logger which we are not using.
                if (logger->name() == "") return;

                ImGui::Text("%s", logger->name().c_str());

                // Let's just hope that this fmt::string_view does terminate data() with a null character.
                const char * current_item = spdlog::level::to_string_view(logger->level()).data();
                if (ImGui::BeginCombo(("##combo" + logger->name()).c_str(), current_item)) // The second parameter is the label previewed before opening the combo.
                {
                    for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                    {
                        bool is_selected = (current_item == items[n]); // You can store your selection however you want, outside or inside your objects
                        if (ImGui::Selectable(items[n], is_selected))
                            // TODO should we check whether the level actually changed instead of setting it each frame?
                            // I suspect it would be trading memory barriers for conditional branching
                            logger->set_level(spdlog::level::from_str(items[n]));
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                    }
                    ImGui::EndCombo();
                }
            });
    }
    ImGui::End();
}


} // namespace snac
} // namespace ad
