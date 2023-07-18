#include "Logging.h"
#include "Profiling.h"
#include "RenderGraph.h"

#include <build_info.h>

#include <graphics/ApplicationGlfw.h>

#include <iostream>

using namespace ad;
using namespace se;

//resource::ResourceFinder makeResourceFinder()
//{
//    filesystem::path assetConfig = platform::getExecutableFileDirectory() / "assets.json";
//    if(exists(assetConfig))
//    {
//        Json config = Json::parse(std::ifstream{assetConfig});
//        
//        // This leads to an ambibuity on the path ctor, I suppose because
//        // the iterator value_t is equally convertible to both filesystem::path and filesystem::path::string_type
//        //return resource::ResourceFinder(config.at("prefixes").begin(),
//        //                                config.at("prefixes").end());
//
//        // Take the silly long way
//        std::vector<std::string> prefixes{
//            config.at("prefixes").begin(),
//            config.at("prefixes").end()
//        };
//        std::vector<std::filesystem::path> prefixPathes;
//        prefixPathes.reserve(prefixes.size());
//        for (auto & prefix : prefixes)
//        {
//            prefixPathes.push_back(std::filesystem::canonical(prefix));
//        }
//        return resource::ResourceFinder(prefixPathes.begin(),
//                                        prefixPathes.end());
//    }
//    else
//    {
//        return resource::ResourceFinder{platform::getExecutableFileDirectory()};
//    }
//}

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

    renderer::RenderGraph renderGraph;

    while (glfwApp.handleEvents())
    {
        PROFILER_BEGIN_FRAME;

        renderGraph.render();
        glfwApp.swapBuffers();

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
