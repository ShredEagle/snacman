#pragma once

#include "EntityWrap.h"
#include "GameContext.h"
#include "Renderer.h"

#include "../../Input.h"

#include <entity/Query.h>

#include <resource/ResourceFinder.h>
#include <platform/Filesystem.h>

#include <imgui.h>

#include <cstdint>
#include <memory>

namespace ad {

namespace graphics { class AppInterface; }
namespace imguiui { class ImguiUi; }
namespace snac {
struct ConfigurableSettings; 
template <class T_renderer> class RenderThread;
}

namespace snacgame {

namespace component {
struct GlobalPose;
struct MappingContext;
struct PoseScreenSpace;
struct Text;
struct VisualModel;
}
namespace system {
class OrbitalCamera; 
class SceneStateMachine;
}
namespace visu { struct GraphicState; }

struct ImguiDisplays
{
    bool mShowMappings = false;
    bool mShowSimulationDelta = false;
    bool mShowImguiDemo = false;
    bool mShowLogLevel = false;
    bool mShowMainProfiler = false;
    bool mShowRenderProfiler = false;
    bool mSpeedControl = false;
    bool mShowPlayerInfo = false;
    bool mShowRenderControls = false;
    bool mPathfinding = false;
    bool mGameControl = false;

    void display()
    {
        ImGui::Begin("Debug windows");
        ImGui::Checkbox("Speed control", &mSpeedControl);
        ImGui::Checkbox("Mappings", &mShowMappings);
        ImGui::Checkbox("Simulation delta", &mShowSimulationDelta);
        ImGui::Checkbox("Log level", &mShowLogLevel);
        ImGui::Checkbox("Main profiler",  &mShowMainProfiler);
        ImGui::Checkbox("Render profiler",  &mShowRenderProfiler);
        ImGui::Checkbox("Player info",  &mShowPlayerInfo);
        ImGui::Checkbox("Render controls",  &mShowRenderControls);
        ImGui::Checkbox("Pathfinding", &mPathfinding);
        ImGui::Checkbox("Game control", &mGameControl);
        ImGui::Checkbox("ImguiDemo", &mShowImguiDemo);
        ImGui::End();
    }
};
/// \brief Implement HidManager's Inhibiter protocol, for Imgui.
/// Allowing the app to discard input events that are handled by DearImgui.
class ImguiInhibiter : public snac::HidManager::Inhibiter
{
public:
    enum WantCapture
    {
        Null,
        Mouse = 1 << 0,
        Keyboard = 1 << 1,
    };

    void resetCapture(WantCapture aCaptures) { mCaptures = aCaptures; }
    bool isCapturingMouse() const override
    {
        return (mCaptures & Mouse) == Mouse;
    }
    bool isCapturingKeyboard() const override
    {
        return (mCaptures & Keyboard) == Keyboard;
    }

private:
    std::uint8_t mCaptures{0};
};

class SnacGame
{
public:
    using Renderer_t = Renderer;

    /// \brief Initialize the scene;
    SnacGame(graphics::AppInterface & aAppInterface,
             snac::RenderThread<Renderer_t> & aRenderThread,
             imguiui::ImguiUi & aImguiUi,
             resource::ResourceFinder aResourceFinder,
             arte::Freetype & aFreetype,
             RawInput & aInput);

    bool update(float aDelta, RawInput & aInput);

    void drawDebugUi(snac::ConfigurableSettings & aSettings,
                     ImguiInhibiter & aInhibiter,
                     RawInput & aInput);

    std::unique_ptr<visu::GraphicState> makeGraphicState();

private:
    graphics::AppInterface * mAppInterface;

    GameContext mGameContext;
    
    // TODO use the ent::Wrap
    EntityWrap<component::MappingContext> mMappingContext; // TODO: should probably be accessed via query
    EntityWrap<system::SceneStateMachine> mStateMachine;
    EntityWrap<system::OrbitalCamera> mSystemOrbitalCamera; // EntityWrap is used to avoid the handle being changed
    EntityWrap<ent::Query<component::GlobalPose, component::VisualModel>> mQueryRenderable;

    EntityWrap<ent::Query<component::Text, component::PoseScreenSpace>> mQueryText;

    // A float would run out of precision too quickly.
    double mSimulationTime{0.};

    imguiui::ImguiUi & mImguiUi;
    ImguiDisplays mImguiDisplays;
};

} // namespace snacgame
} // namespace ad
