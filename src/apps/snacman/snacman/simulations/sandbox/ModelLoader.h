#pragma once

#include <snacman/Input.h>
#include <snacman/Timing.h>

#include <snacman/simulations/snacgame/OrbitalControlInput.h>
#include <snacman/simulations/snacgame/Renderer.h>

#include <resource/ResourceFinder.h>

#include <platform/Filesystem.h>

#include <imgui.h>


namespace ad {

namespace graphics { class AppInterface; }
namespace imguiui { class ImguiUi; }
namespace snac {
    struct ConfigurableSettings; 
    template <class T_renderer> class RenderThread;
}

namespace snacgame {

class ImguiInhibiter;

namespace visu {
    struct GraphicState;
}


class ModelLoader
{
public:
    /// \brief Initialize the scene;
    ModelLoader(graphics::AppInterface & aAppInterface,
                snac::RenderThread<Renderer_t> & aRenderThread,
                imguiui::ImguiUi & aImguiUi,
                resource::ResourceFinder aResourceFinder,
                arte::Freetype & aFreetype,
                RawInput & aInput);

    bool update(snac::Clock::duration & aUpdatePeriod, RawInput & aInput);

    void drawDebugUi(snac::ConfigurableSettings & aSettings,
                     ImguiInhibiter & aInhibiter,
                     RawInput & aInput);

    std::unique_ptr<visu::GraphicState> makeGraphicState();

private:
    struct Model
    {
        std::size_t mIndex;  // for sparse set
        renderer::Handle<const renderer::Object> mObject;
    };

    graphics::AppInterface * mAppInterface;
    imguiui::ImguiUi & mImguiUi;
    OrbitalControlInput mOrbitalControl;
    Model mModel;
};

} // namespace snacgame
} // namespace ad
