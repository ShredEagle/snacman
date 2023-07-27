#include "Logging.h"
#include "Profiling.h"
#include "RenderGraph.h"

#include <build_info.h>

#include <graphics/ApplicationGlfw.h>

#include <iostream>

using namespace ad;
using namespace se;


std::filesystem::path handleArguments(int argc, char * argv[])
{
    if(argc != 2)
    {
        std::cerr << "Usage: " << std::filesystem::path{argv[0]}.filename().string() << " input_model_file\n";
        std::exit(EXIT_FAILURE);
    }

    std::filesystem::path inputPath{argv[1]};
    inputPath.replace_extension(".seum");

    if(!is_regular_file(inputPath))
    {
        std::cerr << "Provided argument should be a file, but '" << inputPath.string() << "' is not.\n";
        std::exit(EXIT_FAILURE);
    }

    return inputPath;
}


void runApplication(int argc, char * argv[])
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

    renderer::RenderGraph renderGraph{
        glfwApp.getAppInterface(),
        handleArguments(argc, argv)
    };

    while (glfwApp.handleEvents())
    {
        PROFILER_BEGIN_FRAME;
        PROFILER_BEGIN_SECTION("frame", renderer::CpuTime, renderer::GpuTime);

        renderGraph.render();
        glfwApp.swapBuffers();

        PROFILER_END_SECTION;
        PROFILER_END_FRAME;

//        std::cout << renderer::getGlobalProfiler().prettyPrint();
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
        runApplication(argc, argv);
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
