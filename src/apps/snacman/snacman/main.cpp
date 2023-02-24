#include "Input.h"
#include "Logging.h"
#include "LoopSettings.h"
#include "Profiling.h"
#include "RenderThread.h"
#include "Timing.h"

#include <build_info.h>

#include "simulations/snacgame/SnacGame.h"


// TODO we should not include something from detail.
// So either move it out of detail, either use nholmann directly
#include <arte/detail/Json.h>

#include <graphics/ApplicationGlfw.h>

#include <imguiui/ImguiUi.h>

#include <math/VectorUtilities.h>

#include <platform/Path.h>

#include <resource/ResourceFinder.h>

#include <fstream>


using namespace ad;
using namespace ad::snac;


// TODO find a better place than global
constexpr bool gWaitByBusyLoop = true;


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
    SELOG(info)("I'm a snac man.");
    SELOG(debug)
    ("Initial delta time {}ms.",
     std::chrono::duration_cast<ms>(gSimulationDelta).count());

    // TODO have this single work (currently return abherent values, even if we put a begin / end frame around the section)
    //BEGIN_SINGLE("App_initialization", appInitSingle);

    // Application and window initialization
    graphics::ApplicationGlfw glfwApp{
        getVersionedName(), 800, 600 // TODO handle via settings
                                    // TODO, applicationFlags
    };

    imguiui::ImguiUi imguiUi(glfwApp);

    ConfigurableSettings configurableSettings;

    //
    // Initialize rendering subsystem
    //

    // Initialize the renderer
    snacgame::Renderer renderer{*glfwApp.getAppInterface()};

    // Context must be removed from this thread before it can be made current on
    // the render thread.
    glfwApp.removeCurrentContext();

    GraphicStateFifo<snacgame::Renderer> graphicStates;
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
    snacgame::SnacGame simulation{
        *glfwApp.getAppInterface(),
        renderingThread,
        imguiUi,
        makeResourceFinder(),
        input};

    //END_SINGLE(appInitSingle);

    //
    // Main simulation loop
    //
    Clock::time_point beginStepTime =
        Clock::now() - configurableSettings.mSimulationDelta;

    while (glfwApp.handleEvents())
    {
        Guard frameProfiling = profileFrame(snac::Profiler::Main);

        BEGIN_RECURRING(Main, "Step", stepRecurringScope);

        // Update input
        input = hid.read(input, inhibiter);

        //
        // Simulate one step
        //
        {
            TIME_RECURRING(Main, "Simulation_update");
            if (simulation.update(
                    (float) asSeconds(configurableSettings.mSimulationDelta),
                    input))
            {
                break;
            }
        }

        simulation.drawDebugUi(configurableSettings, inhibiter, input);
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

        // End the step profiling explicitly here (instead of adding a nesting level)
        END_RECURRING(stepRecurringScope);

        if (!gWaitByBusyLoop)
        {
            if (auto beforeSleep = Clock::now();
                (beginStepTime + configurableSettings.mSimulationDelta)
                > beforeSleep)
            {
                Clock::duration ahead = beginStepTime
                                        + configurableSettings.mSimulationDelta
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
                sleepBusy(configurableSettings.mSimulationDelta, beginStepTime);
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
