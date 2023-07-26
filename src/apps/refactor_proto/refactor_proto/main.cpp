#include "Logging.h"
#include "Profiling.h"
#include "RenderGraph.h"

#include <build_info.h>

#include <graphics/ApplicationGlfw.h>

#include <iostream>

using namespace ad;
using namespace se;


void runApplication()
{
    SELOG(info)("Starting application '{}'.", gApplicationName);

    // Application and window initialization
    graphics::ApplicationFlag glfwFlags = graphics::ApplicationFlag::None;
    graphics::ApplicationGlfw glfwApp{
        getVersionedName(),
        1920, 1024,
        glfwFlags,
        4, 1,
    };

    auto scopeProfiler = renderer::scopeGlobalProfiler();

    renderer::RenderGraph renderGraph;

    while (glfwApp.handleEvents())
    {
        PROFILER_BEGIN_FRAME;
        PROFILER_BEGIN_SECTION("frame", renderer::CpuTime, renderer::GpuTime);

        renderGraph.render(glfwApp);
        glfwApp.swapBuffers();

        PROFILER_END_SECTION;
        PROFILER_END_FRAME;

        std::cout << renderer::getGlobalProfiler().prettyPrint();
    }
    SELOG(info)("Application '{}' is exiting.", gApplicationName);
}


int main(int argc, char * argv[])
{
    // Initialize logging (and use std err if there is an error)
    try
    {
        se::initializeLogging();
        spdlog::set_level(spdlog::level::debug);
    }
    catch (std::exception & aException)
    {
        std::cerr << "Uncaught exception while initializing logging:\n"
                  << aException.what() << std::endl;
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
        SELOG(critical)
        ("Uncaught exception while running application:\n{}",
         aException.what());
        return -1;
    }
    catch (...)
    {
        SELOG(critical)("Uncaught non-exception while running application.");
        return -1;
    }
}
