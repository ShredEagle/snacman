#pragma once

#include "EntityWrap.h"
#include "Renderer.h"

#include "component/Geometry.h"
#include "component/PoseScreenSpace.h"
#include "component/Text.h"

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


namespace ad {
namespace snacgame {

struct ImguiDisplays
{
    bool mShowMappings = false;
    bool mShowSimulationDelta = false;
    bool mShowImguiDemo = false;
    bool mShowLogLevel = false;

    void display()
    {
        ImGui::Begin("Debug windows");
        ImGui::Checkbox("Mappings", &mShowMappings);
        ImGui::Checkbox("Simulation delta", &mShowSimulationDelta);
        ImGui::Checkbox("Log level", &mShowLogLevel);
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
             imguiui::ImguiUi & aImguiUi,
             const resource::ResourceFinder & aResourceFinder);

    bool update(float aDelta, const RawInput & aInput);

    void drawDebugUi(snac::ConfigurableSettings & aSettings,
                     ImguiInhibiter & aInhibiter,
                     const RawInput & aInput);

    std::unique_ptr<visu::GraphicState> makeGraphicState();

    snac::Camera::Parameters getCameraParameters() const;

    // TODO remove finder
    Renderer_t makeRenderer(const resource::ResourceFinder & aResourceFinder) const;

private:
    graphics::AppInterface * mAppInterface;
    snac::Camera::Parameters mCameraParameters = snac::Camera::gDefaults;

    // Must appear before the EntityManager, because the EntityManager
    // might contain Freetype FontFaces, and the Freetype object must outlive them.
    arte::Freetype mFreetype;

    ent::EntityManager mWorld;
    ent::Handle<ent::Entity> mSystems;
    ent::Handle<ent::Entity> mLevel;
    ent::Handle<ent::Entity> mContext;
    EntityWrap<system::OrbitalCamera> mSystemOrbitalCamera;
    EntityWrap<ent::Query<component::Geometry>> mQueryRenderable;

    EntityWrap<ent::Query<component::Text, component::PoseScreenSpace>> mQueryText;

    // A float would run out of precision too quickly.
    double mSimulationTime{0.};

    imguiui::ImguiUi & mImguiUi;
    ImguiDisplays mImguiDisplays;

    bool mHello = false;
};

} // namespace snacgame
} // namespace ad
