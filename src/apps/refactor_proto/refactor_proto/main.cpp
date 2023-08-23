#include "Logging.h"
#include "Profiler.h" // For Profiler::Values
#include "Profiling.h"
#include "RenderGraph.h"
#include "Time.h"

#include <build_info.h>

#include <graphics/ApplicationGlfw.h>
#include <imguiui/ImguiUi.h>

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
    glfwSwapInterval(0); // Disable V-sync

    imguiui::ImguiUi imguiUi{glfwApp};

    auto scopeProfiler = renderer::scopeGlobalProfiler();

    renderer::RenderGraph renderGraph{
        glfwApp.getAppInterface(),
        handleArguments(argc, argv),
        imguiUi,
    };

    renderer::Profiler::Values<std::uint64_t> frameDuration;
    Clock::time_point previousFrame = Clock::now();
    using FrameDurationUnit = std::chrono::microseconds;
    float stepDuration = 0.f;

    std::ostringstream profilerOut;

    while (glfwApp.handleEvents())
    {
        PROFILER_BEGIN_FRAME;
        PROFILER_PUSH_SECTION("frame", renderer::CpuTime, renderer::GpuTime);

        {
            PROFILER_SCOPE_SECTION("fps_counter", renderer::CpuTime, renderer::GpuTime);
            {
                // TODO this could be integrated into the Profiler::beginFrame()
                auto startTime = Clock::now();
                frameDuration.record(getTicks<FrameDurationUnit>(startTime - previousFrame));
                stepDuration = (float)asFractionalSeconds(startTime - previousFrame);
                previousFrame = startTime;
            }
            if( auto framePeriodUs = frameDuration.average();
                framePeriodUs != 0)
            {
                float fps = (float)(FrameDurationUnit::period::den) / (framePeriodUs * FrameDurationUnit::period::num);
                std::ostringstream titleOss;
                titleOss.precision(2);
                titleOss << getVersionedName() 
                    << " (" << std::fixed << fps << " fps | " << framePeriodUs / 1000.f << " ms/f)"
                    ;
                glfwApp.setWindowTitle(titleOss.str());
            }
        }

        renderGraph.update(stepDuration);
        renderGraph.render();
        imguiUi.newFrame();
        ImGui::Begin("Profiler");
        ImGui::Text(profilerOut.str().c_str());
        ImGui::End();
        imguiUi.render();
        imguiUi.renderBackend();
        glfwApp.swapBuffers();

        PROFILER_POP_SECTION;
        PROFILER_END_FRAME;

        // TODO Ad 2023/08/22: With this structure, it is showing the profile from previous frame
        // would be better to show current frame (but this implies to end the profiler frame earlier)
        profilerOut.str(""); // reset to empty string
        PROFILER_PRINT_TO_STREAM(profilerOut);
    }
    SELOG(info)("Application '{}' is exiting.", gApplicationName);
}


int main(int argc, char * argv[])
{
    // Initialize logging (and use std err if there is an error)
    try
    {
        se::initializeLogging();
        spdlog::set_level(spdlog::level::info);
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
