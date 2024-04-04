#include "DebugDrawing.h"
#include "DevmodeControl.h"
#include "Input.h"
#include "Logging.h"
#include "LoopSettings.h"
#include "Profiling.h"
#include "RenderThread.h"
#include "Timing.h"

#include <build_info.h>

#include "simulations/sandbox/ModelLoader.h"

#include "simulations/snacgame/ImguiInhibiter.h"
#include "simulations/snacgame/SnacGame.h"
#include "simulations/snacgame/Renderer.h"

// TODO we should not include something from detail.
// So either move it out of detail, either use nholmann directly
#include <arte/detail/Json.h>

#include <graphics/ApplicationGlfw.h>

#include <imguiui/ImguiUi.h>

#include <platform/Path.h>

#include <resource/ResourceFinder.h>

// TODO remove
#include <snac-renderer-V1/ResourceLoad.h>

// For fine-tuning the log-levels.
#include <snac-renderer-V2/Logging-channels.h>
// TODO replace by an include not scoped to renderer-V2
#include <snac-renderer-V2/Profiling.h>

#include <fstream>

using namespace ad;
using namespace ad::snac;


// TODO find a better place than global
constexpr bool gWaitByBusyLoop = true;


using Simulation_t = snacgame::SnacGame;
//using Simulation_t = snacgame::ModelLoader;


resource::ResourceFinder makeResourceFinder()
{
    filesystem::path assetConfig = platform::getExecutableFileDirectory() / "assets.json";
    if(exists(assetConfig))
    {
        Json config = Json::parse(std::ifstream{assetConfig});
        
        // This leads to an ambibuity on the path ctor, I suppose because
        // the iterator value_t is equally convertible to both filesystem::path and filesystem::path::string_type
        //return resource::ResourceFinder(config.at("prefixes").begin(),
        //                                config.at("prefixes").end());

        // Take the silly long way
        std::vector<std::string> prefixes{
            config.at("prefixes").begin(),
            config.at("prefixes").end()
        };
        std::vector<std::filesystem::path> prefixPathes;
        prefixPathes.reserve(prefixes.size());
        for (auto & prefix : prefixes)
        {
            prefixPathes.push_back(std::filesystem::canonical(prefix));
        }
        return resource::ResourceFinder(prefixPathes.begin(),
                                        prefixPathes.end());
    }
    else
    {
        return resource::ResourceFinder{platform::getExecutableFileDirectory()};
    }
}


void showDiagnostics()
{
    SELOG(info)("Max uniform block size: {} bytes. Max vertex uniform blocks: {}.",
                graphics::getInt(GL_MAX_UNIFORM_BLOCK_SIZE),
                graphics::getInt(GL_MAX_VERTEX_UNIFORM_BLOCKS) // the limit of uniform buffer binding locations.
    );
}


void runApplication()
{
    SELOG(info)("I'm a snac man.");
    SELOG(debug)
    ("Initial simulation perdiod {}ms.",
     std::chrono::duration_cast<ms>(gSimulationPeriod).count());

    initializeDebugDrawers();

    // TODO have this single work (currently return abherent values, even if we put a begin / end frame around the section)
    //BEGIN_SINGLE("App_initialization", appInitSingle);

    // Application and window initialization
    graphics::ApplicationFlag glfwFlags = graphics::ApplicationFlag::None;
    if(!isDevmode())
    {
        glfwFlags |= graphics::ApplicationFlag::Fullscreen;
    }
    graphics::ApplicationGlfw glfwApp{
        getVersionedName(),
        1920, 1024,
        glfwFlags,// TODO, handle applicationFlags via settings
        4, 1,
        { {GLFW_SAMPLES, 4} },
    };

    // Must be scoped below the GL context.
    auto mainProfilerScope = SCOPE_PROFILER(gMainProfiler, renderer::Profiler::Providers::CpuOnly);

    // Ensures the messages are sent synchronously with the event triggering them
    // This makes debug stepping much more feasible.
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    showDiagnostics();

    imguiui::ImguiUi imguiUi(glfwApp);

    ConfigurableSettings configurableSettings;

    // Must outlive all FontFaces: 
    // * it must outlive the EntityManager (which might contain Freetype FontFaces)
    // * it must outlive the resource managers holding fonts
    // * it must outlive the RenderThread, holding fonts
    // TODO Design a clearer approach to "long lived library objects"
    // This has been a recurring problem, moving this freetype instance up each time to try to outlive all the things.
    arte::Freetype freetype;

    resource::ResourceFinder finder = makeResourceFinder();

    //
    // Initialize rendering subsystem
    //

    // Initialize the renderer
    // TODO we provide a Load<Technique> so the shadow pipeline can use it to load the effects for its cube.
    // this complicates the interface a lot, and since it does not use the ResourceManager those effects are not hot-recompilable.
    snac::TechniqueLoader techniqueLoader{finder};
    snacgame::Renderer_t renderer{
        *glfwApp.getAppInterface(),
         techniqueLoader,
         freetype.load(finder.pathFor("fonts/FiraMono-Regular.ttf"))
    };

    // Context must be removed from this thread before it can be made current on
    // the render thread.
    glfwApp.removeCurrentContext();

    GraphicStateFifo<snacgame::Renderer_t> graphicStates;
    RenderThread renderingThread{
        glfwApp,
        graphicStates,
        std::move(renderer),
        imguiUi,
        configurableSettings
    };

    //
    // Initialize input devices
    //
    HidManager hid{glfwApp};
    RawInput input = hid.initialInput();
    snacgame::ImguiInhibiter inhibiter;

    //
    // Initialize scene
    //
    Simulation_t simulation{
        *glfwApp.getAppInterface(),
        renderingThread,
        imguiUi,
        std::move(finder),
        freetype,
        input};

    //END_SINGLE(appInitSingle);

    //
    // Main simulation loop
    //

    // TODO we might extend the Time structure to handle user wall-time
    // and use this facility to handle simulation steps pacing.
    Clock::time_point beginStepTime =
        Clock::now() - configurableSettings.mSimulationPeriod;

    while (glfwApp.handleEvents())
    {
        // TODO Ad 2024/04/02: #profilerv1 Remove the old profiler frame guard
        Guard stepProfiling = profileFrame(getProfiler(snac::Profiler::Main));
        
        auto v2PrintScope = BEGIN_RECURRING_V1(Main, "profiler_v2_print");
        // With profiler V2, we cannot print the results while inside a profiler's frame.
        std::ostringstream profilerOutput;
        PROFILER_PRINT_TO_STREAM(snac::gMainProfiler, profilerOutput);
        END_RECURRING_V1(v2PrintScope);

        Guard scopedMainFrameProfiling = renderer::scopeProfilerFrame(gMainProfiler);
        auto stepRecurringScope = BEGIN_RECURRING(Main, "Step");

        if constexpr(isDevmode())
        {
            simulation.drawDebugUi(configurableSettings, inhibiter, input, profilerOutput.str());
        }
        else
        {
            // Render empty frame, so renderthread can still call renderBackend()
            std::lock_guard lock{imguiUi.mFrameMutex};
            imguiUi.newFrame();
            imguiUi.render();
        }

        // Update input
        input = hid.read(input, inhibiter);

        //
        // Simulate one step
        //
        {
            TIME_RECURRING(Main, "Simulation_update");
            if (simulation.update(
                    configurableSettings.mSimulationPeriod,
                    input))
            {
                break;
            }
        }

        // Pretend update took longer if requested by user.
        sleepBusy(configurableSettings.mUpdateDuration, beginStepTime);

        //
        // Push the graphic state for the latest simulated state
        //
        graphicStates.push(simulation.makeGraphicState());

        // Regularly check if the rendering thread did not throw
        renderingThread.checkRethrow();

        //
        // Release CPU cycles until next time point to advance the simulation.
        //
        // Note: The wait happens **before** handleEvents.
        // If it happened between handleEvents and scene.update(),
        // the simulation could get unnecessarily "outdated" input.

        // Update the simulation delta at this point.
        // If it was updated before scene.update(), there would be an
        // inconsistency on each new delta value. (Because an update for
        // duration D2 would be presented after a duration of D1.)

        // End the step profiling explicitly here (instead of adding a brace nesting level)
        END_RECURRING(stepRecurringScope);

        if (!gWaitByBusyLoop)
        {
            if (auto beforeSleep = Clock::now();
                (beginStepTime + configurableSettings.mSimulationPeriod)
                > beforeSleep)
            {
                Clock::duration ahead = beginStepTime
                                        + configurableSettings.mSimulationPeriod
                                        - beforeSleep;
                // Could be sleep_until
                std::this_thread::sleep_for(ahead);

                SELOG(trace)
                ("Expected to sleep for {}ms, actually slept for {}ms",
                 std::chrono::duration_cast<ms>(ahead).count(),
                 std::chrono::duration_cast<ms>(Clock::now() - beforeSleep)
                     .count());
            }
        }
        else
        {
            TIME_RECURRING(Main, "busy_wait");
            SleepResult slept =
                sleepBusy(configurableSettings.mSimulationPeriod, beginStepTime);
            SELOG(trace)
            ("Expected to sleep for {}ms, actually slept for {}ms",
             std::chrono::duration_cast<ms>(slept.targetDuration).count(),
             std::chrono::duration_cast<ms>(Clock::now() - slept.timeBefore)
                 .count());
        }

        //
        // Note the beginning time of next step
        // (this correctly includes handleEvent() as part of next step)
        //
        beginStepTime = Clock::now();
    }

    // Stop and join the thread
    renderingThread.finalize();
}


int main(int argc, char * argv[])
{
    // Initialize logging (and use std err if there is an error)
    try
    {
        snac::detail::initializeLogging();
        spdlog::set_level(spdlog::level::debug);

        // Opportunity to control the default log-level per-logger
        //spdlog::get(renderer::gPipelineDiag)->set_level(spdlog::level::critical);
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
