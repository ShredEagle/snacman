#include "Logging.h"
#include "Scene.h"

#include <build_info.h>

#include <snac-renderer/Render.h>

#include <graphics/ApplicationGlfw.h>

//#include <imguiui/ImguiUi.h>

using namespace ad;
using namespace ad::snac;


void runApplication()
{
    SELOG(info)(getVersionedName());

    // Application and window initialization
    graphics::ApplicationGlfw glfwApp{getVersionedName(),
                                      800, 600 // TODO handle via settings
                                      //TODO, applicationFlags
    };

    // Match the viewport to the framebuffer size.
    auto viewportListening = glfwApp.getAppInterface()->listenFramebufferResize(
        [](const math::Size<2, int> & size){ glViewport(0, 0, size.width(), size.height()); });

    //
    // Initialize scene
    //
    Scene scene;
    Renderer renderer;

    //
    // Main simulation loop
    //
    //Clock::time_point beginStepTime = Clock::now() - gImguiGameLoop.getSimulationDelta();
    while(glfwApp.handleEvents())
    {
        //Clock::time_point previousStepTime = beginStepTime;

        //
        // Simulate one step
        //
        //beginStepTime = Clock::now();
        //scene.update((float)asSeconds(simulationDelta));

        //
        // Render
        //
        clear();
        renderer.render(scene.mMesh, scene.mCamera);
        glfwApp.swapBuffers();
    }
}


int main(int argc, char * argv[])
{
    // Initialize logging (and use std err if there is an error)
    try
    {
        snac::detail::initializeLogging();
        spdlog::set_level(spdlog::level::trace);
    }
    catch (std::exception & aException)
    {
        std::cerr << "Uncaught exception while initializing logging:\n"
            <<   aException.what()
            << std::endl;
        return -2;
    }
    catch (...)
    {
        std::cerr << "Uncaught non-exception while initializing logging."
            << std::endl;
        return -1;
    }

    // Run the application (and use logging facilities if there is an error)
    try
    {
        runApplication();
    }
    catch (std::exception & aException)
    {
        SELOG(critical)("Uncaught exception while running application:\n{}",
                        aException.what());
        return -1;
    }
    catch (...)
    {
        SELOG(critical)("Uncaught non-exception while running application.");
        return -1;
    }
}
