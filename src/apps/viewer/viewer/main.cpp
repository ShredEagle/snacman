#include "Logging.h"
#include "Timing.h"
#include "ViewerApplication.h"

#include <build_info.h>

#include <graphics/ApplicationGlfw.h>

#include <imguiui/ImguiUi.h>
#include <imguiui/Widgets.h>

#include <ittnotify.h>

#include <profiler/GlApi.h>
#include <profiler/Profiler.h> // Internals are used to measure frame duration

#include <snac-renderer-V2/debug/DebugDrawing.h>
#include <snac-renderer-V2/Profiling.h>

#include <utilities/Time.h>

#include <CLI/CLI.hpp>

#include <iostream>


using namespace ad;
using namespace se;

using renderer::gRenderProfiler;


struct Arguments
{
    std::filesystem::path mSceneFile;
    std::optional<std::filesystem::path> mCubemapEnv;
    std::optional<std::filesystem::path> mEquirectangularEnv;
};


/// @brief Handle the command line arguments
/// @return 0 if the arguments were accepted, not-0 otherwise.
int handleArguments(int argc, char * argv[], Arguments & aArgs)
{
    CLI::App cliApp{"ShredEagle 3D model viewer."};
    argv = cliApp.ensure_utf8(argv);
    
    cliApp.add_option("sew-file", aArgs.mSceneFile,
                      "The scene file containing the model to load.")
        ->required();

    cliApp.add_option("-c, --cube-environment", aArgs.mCubemapEnv,
                      "Environment map, given as a cubemap strip. Takes precedence.");

    cliApp.add_option("-e, --equirectangular-environment", aArgs.mEquirectangularEnv,
                      "Environment map, given as an equirectangular map.");

    CLI11_PARSE(cliApp, argc, argv);

    // TODO Study how CLI11 advises to do arguments validation
    if(!is_regular_file(aArgs.mSceneFile))
    {
        std::cerr << "Provided scene should be a file, but '" << aArgs.mSceneFile.string() << "' is not.\n";
        return EXIT_FAILURE;
    }

    return 0;
}


void showGui(imguiui::ImguiUi & imguiUi,
             renderer::ViewerApplication & aApplication,
             const std::string & aProfilerOutput,
             const renderer::Timing & aTime)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "imgui main ui", renderer::CpuTime, renderer::GpuTime);

    assert(ImGui::GetCurrentContext() == nullptr);

    imguiUi.newFrame();

    ImGui::Begin("Main control");
    {
        aApplication.drawMainUi(aTime);

        static bool showProfiler = false;
        if(imguiui::addCheckbox("Profiler", showProfiler))
        {
            ImGui::Begin("Profiler");
            ImGui::Text("%s", aProfilerOutput.c_str());
            ImGui::End();
        }

        static bool showGlMetrics = false;
        if(imguiui::addCheckbox("GL Metrics", showGlMetrics))
        {
            ImGui::Begin("GL metrics");
            std::ostringstream metricsOs;
            metricsOs << "Buffer memory:" 
                << "\n\tallocated:" << ad::renderer::gl.get().mBufferMemory.mAllocated / 1024 << " kB."
                << "\n\twritten:"   << ad::renderer::gl.get().mBufferMemory.mWritten / 1024 << " kB."
                << "\n\nTexture memory:"
                << "\n\tallocated:" << ad::renderer::gl.get().mTextureMemory.mAllocated / 1024 << " kB."
                << "\n\twritten:"   << ad::renderer::gl.get().mTextureMemory.mWritten / 1024 << " kB."
                ;
            ImGui::Text("%s", metricsOs.str().c_str());
            ImGui::End();
        }

        static bool showDemo = false;
        if(imguiui::addCheckbox("ImGui demo", showDemo))
        {
            ImGui::ShowDemoWindow();
        }
    }
    ImGui::End();

    imguiUi.render();
    imguiUi.renderBackend();

    // We want to make sure the context is always explicitly set by our ImguiUi implementation
    ImGui::SetCurrentContext(nullptr);
}


void showSecondGui(imguiui::ImguiUi & imguiUi,
                   renderer::ViewerApplication & aApplication)
{
    PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "imgui secondary ui", renderer::CpuTime);

    assert(ImGui::GetCurrentContext() == nullptr);

    imguiUi.newFrame();

    ImGui::Begin("Debug");
    {
        aApplication.drawSecondaryUi();
    }
    ImGui::End();

    imguiUi.render();
    imguiUi.renderBackend();

    // We want to make sure the context is always explicitly set by our ImguiUi implementation
    ImGui::SetCurrentContext(nullptr);
}


int runApplication(int argc, char * argv[])
{
    SELOG(info)("Starting application '{}'.", gApplicationName);

    __itt_domain * itt_domain = __itt_domain_create("se::viewer");

    // CLI arguments
    Arguments args;
    if(int result = handleArguments(argc, argv, args))
    {
        return result;
    }

    // DebugDrawer
    renderer::initializeDebugDrawers();

    // Instantiate main window
    constexpr unsigned int gMsaaSamples = 1;

    __itt_string_handle* itt_handleGlfwApp = __itt_string_handle_create("glfwApp-ctor");
    graphics::ApplicationFlag glfwFlags = graphics::ApplicationFlag::None;
    __itt_task_begin(itt_domain, __itt_null, __itt_null, itt_handleGlfwApp);
    graphics::ApplicationGlfw glfwApp{
        getVersionedName(),
        1280, 1024,
        glfwFlags,
        4, 6,
        { {GLFW_SAMPLES, gMsaaSamples} },
    };
    glfwSwapInterval(0); // Disable V-sync
    __itt_task_end(itt_domain);

    // Ensures the messages are sent synchronously with the event triggering them
    // This makes debug stepping much more feasible.
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    // Associate an ImguiUi to the main window
    imguiui::ImguiUi imguiUi{glfwApp};
    imguiUi.registerGlfwCallbacks(glfwApp);


    // Instantiate 2nd window
    auto secondWindow = graphics::ApplicationGlfw{glfwApp, "Secondary", 800, 600};
    glfwSwapInterval(0); // Disable V-sync
    renderer::ViewBlitter blitter; // Created in the GL context of the destination window

    // Immediately make the main window context current
    // It is essential when generating queries, which apparently are not shared
    // Also, SecondaryView initialize objects that have to be on the main context
    glfwApp.makeContextCurrent();

    // Associate an ImguiUi to the 2nd window
    imguiui::ImguiUi secondImguiUi{secondWindow};
    secondImguiUi.registerGlfwCallbacks(secondWindow);

    auto renderProfilerScope = SCOPE_PROFILER(gRenderProfiler, renderer::Profiler::Providers::All);

    // Instantiate the ViewerApplication, that will load assets
    auto loadingSection = PROFILER_BEGIN_SINGLESHOT_SECTION(gRenderProfiler, , "rendergraph_loading", renderer::CpuTime);
    __itt_string_handle* itt_handleLoadingSection = __itt_string_handle_create("loadingSection");
    __itt_task_begin(itt_domain, __itt_null, __itt_null, itt_handleLoadingSection);
    renderer::ViewerApplication application{
        glfwApp.getAppInterface(),
        secondWindow.getAppInterface(),
        args.mSceneFile,
        imguiUi,
        secondImguiUi
    };
    if(auto environment = args.mCubemapEnv)
    {
        application.setEnvironmentCubemap(*environment);
    }
    else if(auto environment = args.mEquirectangularEnv)
    {
        application.setEnvironmentEquirectangular(*environment);
    }
    __itt_task_end(itt_domain);
    PROFILER_END_SECTION(loadingSection);

    renderer::Timing timing;

    renderer::Profiler::Values<std::uint64_t> frameDuration;
    Clock::time_point previousFrame = Clock::now();
    using FrameDurationUnit = std::chrono::microseconds;
    float stepDuration = 0.f;

    // Used as memory from one call to the next
    std::ostringstream profilerOut;

    // Main loop
    while (glfwApp.handleEvents())
    {
        PROFILER_BEGIN_FRAME(gRenderProfiler);
        auto frameSection = PROFILER_BEGIN_RECURRING_SECTION(gRenderProfiler, "frame", renderer::CpuTime, renderer::GpuTime);

        {
            PROFILER_SCOPE_RECURRING_SECTION(gRenderProfiler, "fps_counter", renderer::CpuTime);
            {
                // TODO this could be integrated into the Profiler::beginFrame()
                // Currently measure the real frame time (not just the time between beginFrame()/endFrame())
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

        application.update(timing.advance(stepDuration));
        application.render();

        // TODO Ad 2023/08/22: With this structure, it is showing the profile from previous frame
        // would be better to show current frame 
        // (but this implies to end the profiler frame earlier, thus not profiling ImGui UI)
        {
            graphics::ScopedBind boundDefaultFb{graphics::FrameBuffer::Default()};
            showGui(imguiUi, application, profilerOut.str(), timing);
        }

        glfwApp.swapBuffers();

        PROFILER_END_SECTION(frameSection);
        PROFILER_END_FRAME(gRenderProfiler);

        // Handle the second window: closing it does not terminate the app, but hide the window
        if(!secondWindow.shouldClose())
        {
            if(secondWindow.isVisible())
            {
                // Make the second window OpenGL context current, to fill its default framebuffer.
                secondWindow.makeContextCurrent();
                //glClear(GL_COLOR_BUFFER_BIT); // Useless, we blit the whole framebuffer
                blitter.blitIt(application.mSecondaryView.mColorBuffer,
                               math::Rectangle<GLint>::AtOrigin(application.mSecondaryView.mRenderSize),
                               graphics::FrameBuffer::Default());
                // Prepare the GUI for the 2nd window.
                showSecondGui(secondImguiUi, application);
                secondWindow.swapBuffers();
                glfwApp.makeContextCurrent();
            }
        }
        else
        {
            secondWindow.hide();
            secondWindow.markWindowShouldClose(GLFW_FALSE);
        }

        // Note: the printing of the profiler content happens out of the frame, so its time is excluded from profiler's frame time
        profilerOut.str("");
        PROFILER_PRINT_TO_STREAM(gRenderProfiler, profilerOut);
    }
    SELOG(info)("Application '{}' is exiting.", gApplicationName);

    return 0;
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
        return runApplication(argc, argv);
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
