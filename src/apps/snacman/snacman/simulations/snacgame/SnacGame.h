#pragma once

#include "EntityWrap.h"
#include "GameContext.h"
#include "Renderer.h"

#include "component/Geometry.h"
#include "component/PoseScreenSpace.h"
#include "component/Text.h"
#include "component/VisualMesh.h"

#include "scene/Scene.h"

#include "snacman/LoopSettings.h"
#include "component/Context.h"
#include "system/SceneStateMachine.h"
#include "system/SystemOrbitalCamera.h"

#include "../../Input.h"
#include "../../LoopSettings.h"
#include "../../Timing.h"

#include <arte/Freetype.h>

#include <entity/EntityManager.h>

#include <graphics/AppInterface.h>

#include <imguiui/ImguiUi.h>

#include <markovjunior/Interpreter.h>

#include <resource/ResourceFinder.h>

#include <memory>

#include <platform/Filesystem.h>

namespace ad {
namespace snacgame {

// TODO: before commit change this
constexpr int gGridSize = 29;
constexpr float gCellSize = 2.f;

struct ImguiDisplays
{
    bool mShowMappings = false;
    bool mShowSimulationDelta = false;
    bool mShowImguiDemo = false;
    bool mShowLogLevel = false;
    bool mShowMainProfiler = false;
    bool mShowRenderProfiler = false;

    void display()
    {
        ImGui::Begin("Debug windows");
        ImGui::Checkbox("Mappings", &mShowMappings);
        ImGui::Checkbox("Simulation delta", &mShowSimulationDelta);
        ImGui::Checkbox("Log level", &mShowLogLevel);
        ImGui::Checkbox("Main profiler",  &mShowMainProfiler);
        ImGui::Checkbox("Render profiler",  &mShowRenderProfiler);
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
    EntityWrap<ent::Query<component::Geometry, component::VisualMesh>> mQueryRenderable;

    EntityWrap<ent::Query<component::Text, component::PoseScreenSpace>> mQueryText;

    // A float would run out of precision too quickly.
    double mSimulationTime{0.};

    imguiui::ImguiUi & mImguiUi;
    ImguiDisplays mImguiDisplays;
};

} // namespace snacgame
} // namespace ad
