#pragma once

#include "EntityWrap.h"
#include "GameContext.h"
#include "Renderer_V2.h"
#include "ImguiSceneEditor.h"

#include "component/PlayerSlot.h"
#include "component/PlayerGameData.h"

#include <snacman/Input.h>
#include <snacman/Timing.h>

#include <entity/Query.h>
#include <entity/Wrap.h>

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

class ImguiInhibiter;

namespace component {
struct GlobalPose;
struct MappingContext;
struct PoseScreenSpace;
struct Text;
struct VisualModel;
struct PlayerHud;
}
namespace system {
class OrbitalCamera; 
struct SceneStack;
}
namespace visu { struct GraphicState; }

struct ImguiDisplays
{
    bool mShowSceneEditor = false;
    bool mShowMappings = false;
    bool mShowSimulationDelta = false;
    bool mShowImguiDemo = false;
    bool mShowRoundInfo = false;
    bool mShowLogLevel = false;
    bool mShowDebugDrawers = false;
    bool mShowMainProfiler = false;
    bool mShowRenderProfiler = false;
    bool mSpeedControl = false;
    bool mShowPlayerInfo = false;
    bool mShowRenderControls = false;
    bool mPathfinding = false;
    bool mGameControl = false;
    bool mTestSerialization = false;

    void display()
    {
        ImGui::Begin("Debug windows");
        ImGui::Checkbox("Scene editor",  &mShowSceneEditor);
        ImGui::Checkbox("Speed control", &mSpeedControl);
        ImGui::Checkbox("Mappings", &mShowMappings);
        ImGui::Checkbox("Simulation delta", &mShowSimulationDelta);
        ImGui::Checkbox("Log level", &mShowLogLevel);
        ImGui::Checkbox("Debug drawers", &mShowDebugDrawers);
        ImGui::Checkbox("Main profiler",  &mShowMainProfiler);
        ImGui::Checkbox("Render profiler",  &mShowRenderProfiler);
        ImGui::Checkbox("Player info",  &mShowPlayerInfo);
        ImGui::Checkbox("Round info",  &mShowRoundInfo);
        ImGui::Checkbox("Render controls",  &mShowRenderControls);
        ImGui::Checkbox("Pathfinding", &mPathfinding);
        ImGui::Checkbox("Game control", &mGameControl);
        ImGui::Checkbox("Test serialization", &mTestSerialization);
        ImGui::Checkbox("ImguiDemo", &mShowImguiDemo);
        ImGui::End();
    }
};

class SnacGame
{
public:
    /// \brief Initialize the scene;
    SnacGame(graphics::AppInterface & aAppInterface,
             snac::RenderThread<Renderer_t> & aRenderThread,
             imguiui::ImguiUi & aImguiUi,
             resource::ResourceFinder aResourceFinder,
             arte::Freetype & aFreetype,
             RawInput & aInput);

    bool update(snac::Clock::duration & aUpdatePeriod, RawInput & aInput);

    void drawDebugUi(snac::ConfigurableSettings & aSettings,
                     ImguiInhibiter & aInhibiter,
                     RawInput & aInput,
                     const std::string & aProfilerResults);

    std::unique_ptr<Renderer_t::GraphicState_t> makeGraphicState();

private:
    graphics::AppInterface * mAppInterface;

    GameContext mGameContext;
    
    // TODO use the ent::Wrap
    ent::Wrap<component::MappingContext> mMappingContext; // TODO: should probably be accessed via query
    EntityWrap<system::OrbitalCamera> mSystemOrbitalCamera; // EntityWrap is used to avoid the handle being changed

    EntityWrap<ent::Query<component::GlobalPose, component::VisualModel>> mQueryRenderable;
    EntityWrap<ent::Query<component::Text, component::GlobalPose>> mQueryTextWorld;
    EntityWrap<ent::Query<component::Text, component::PoseScreenSpace>> mQueryTextScreen;
    EntityWrap<ent::Query<component::PlayerHud>> mQueryHuds;

    imguiui::ImguiUi & mImguiUi;
    ImguiDisplays mImguiDisplays;
    SceneEditor mSceneEditor;

    snac::Time mSimulationTime;
};

} // namespace snacgame
} // namespace ad
