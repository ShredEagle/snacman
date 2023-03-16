#include "Logging.h"
#include "Scene.h"

#include <build_info.h>

#include <arte/detail/Json.h>

#include <graphics/ApplicationGlfw.h>

#include <platform/Path.h>

#include <resource/ResourceFinder.h>

#include <snac-renderer/Render.h>

#include <fstream>


using namespace ad;
using namespace ad::snac;


resource::ResourceFinder makeResourceFinder()
{
    filesystem::path assetConfig = platform::getExecutableFileDirectory() / "assets.json";
    if(exists(assetConfig))
    {
        Json config = Json::parse(std::ifstream{assetConfig});
        
        // Take the silly long way
        std::vector<std::string> prefixes{
            config.at("prefixes").begin(),
            config.at("prefixes").end()
        };
        return resource::ResourceFinder(prefixes.begin(),
                                        prefixes.end());
    }
    else
    {
        return resource::ResourceFinder{platform::getExecutableFileDirectory()};
    }
}


void runApplication()
{
    SELOG(info)(getVersionedName());

    // Application and window initialization
    graphics::ApplicationGlfw glfwApp{getVersionedName(),
                                      800, 600 // TODO handle via settings
                                      //TODO, applicationFlags
    };

    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    resource::ResourceFinder finder = makeResourceFinder();

    //
    // Initialize scene
    //

    Scene scene{
        glfwApp,
        //loadModel(finder.pathFor("Box/glTF/Box.gltf"),
        loadModel(finder.pathFor("Avocado/glTF/Avocado.gltf"),
                  loadEffect(finder.pathFor("effects/MeshTextures.sefx"), finder)),
        finder,
    };
    RendererAlt renderer;

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
        scene.update();

        //
        // Render
        //
        scene.render(renderer);
        glfwApp.swapBuffers();
    }
}


int main(int argc, char * argv[])
{
    // Initialize logging (and use std err if there is an error)
    try
    {
        snac::detail::initializeLogging();
        spdlog::set_level(spdlog::level::debug);
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
