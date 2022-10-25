#include "Logging.h"
#include "Timing.h"
#include "GraphicState.h"

#include "bawls/Bawls.h"
#include "bawls/Renderer.h"

#include <build_info.h>

#include <graphics/ApplicationGlfw.h>

#include <imguiui/ImguiUi.h>

#include <queue>


using namespace ad;
using namespace ad::snac;


using ms = std::chrono::milliseconds;

//constexpr Clock::duration gSimulationDelta = 1.f/60;
constexpr Clock::duration gSimulationDelta = ms{50};

constexpr bool gWaitByBusyLoop = true;

using GraphicStateFifo = StateFifo<bawls::GraphicState>;


class ImguiGameLoop
{
public:
    void render()
    {
        int simulationPeriod = (int)mSimulationPeriodMs;
        if(ImGui::InputInt("Simulation period (ms)", &simulationPeriod))
        {
            simulationPeriod = std::clamp(simulationPeriod, 1, 200);
        }
        mSimulationPeriodMs = simulationPeriod;
    }

    Clock::duration getSimulationDelta() const
    {
        return Clock::duration{ms{mSimulationPeriodMs}};
    }

private:
    std::atomic<Clock::duration::rep> mSimulationPeriodMs{duration_cast<ms>(gSimulationDelta).count()};
};


// TODO find a better place than global
ImguiGameLoop gImguiGameLoop;


class RenderThread
{
public:
    RenderThread(graphics::ApplicationGlfw & aGlfwApp,
                 GraphicStateFifo & aStates,
                 math::Size<2, GLfloat> aWindowSize_world) :
        mApplication{aGlfwApp}
    {
        mThread = std::thread{
            std::bind(&RenderThread::run,
                      this,
                      std::ref(aStates),
                      aWindowSize_world)};
    }

    /// \brief Stop the thread and rethrow if it actually threw.
    ///
    /// A destructor should never throw, so we cannot implement this behaviour in the destructor. 
    void finalize()
    {
        stop();
        checkRethrow();
    }

    void resizeViewport(math::Size<2, int> aFramebufferSize)
    {
        std::lock_guard lock{mOperationsMutex};
        mOperations.push([=]()
            {
                glViewport(0, 0, aFramebufferSize.width(), aFramebufferSize.height());
            });
    }

    void checkRethrow()
    {
        if (mThrew)
        {
            std::rethrow_exception(mThreadException);
        }
    }
    

private:
    void stop()
    {
        mStop = true;
        mThread.join();
    }

    void run(GraphicStateFifo & aStates, math::Size<2, GLfloat> aWindowSize_world)
    {
        try
        {
            run_impl(aStates, aWindowSize_world);
        }
        catch(...)
        {
            mThreadException = std::current_exception();
            mThrew = true;
        }
    }

    void run_impl(GraphicStateFifo & aStates, math::Size<2, GLfloat> aWindowSize_world)
    {
        SELOG(info)("Render thread started");

        // The context must be made current on this thread before it can call GL functions.
        mApplication.makeContextCurrent();


        //
        // Imgui
        //
        imguiui::ImguiUi ui{mApplication};
        bool showImguiDemo = true;

        // Get a first entry
        auto entry = [&]()
        {
            while(true)
            {
                if (auto firstState = aStates.popToEnd(); firstState)
                {
                    return std::move(*firstState);
                }
            }
        }();
        SELOG(info)("Render thread retrieved first state.");

        // Must be initialized here, in the render thread, because the ctor makes
        // OpenGL calls (and the GL context is current on the render thread).
        bawls::Renderer renderer{aWindowSize_world};

        while(!mStop)
        {
            // Service all queued operations first
            {
                std::lock_guard lock{mOperationsMutex};
                while(!mOperations.empty())
                {
                    mOperations.front()();
                    mOperations.pop();
                }
            }

            // TODO simulate delay in the render thread:
            // * Thread iteration time (simulate what happens on the thread, potentially visibility).
            // * GPU load, i.e. rendering time. This might be harder to simulate.
            //std::this_thread::sleep_for(ms{8});

            // TODO interpolate

            mApplication.getAppInterface()->clear();
            renderer.render(*entry.state);
            SELOG(trace)("Render thread: Frame rendered.");

            ui.newFrame();

            ImGui::ShowDemoWindow(&showImguiDemo);
            ImGui::Begin("Gameloop");
            //ImGui::Checkbox("Demo window", &showImguiDemo);
            gImguiGameLoop.render();
            ImGui::End();

            ui.render();

            mApplication.swapBuffers();

            // Get new latest state, if any
            if (auto latest = aStates.popToEnd(); latest)
            {
                entry = std::move(*latest);
                SELOG(trace)("Render thread: Newer state retrieved.");
            }
        }

        SELOG(info)("Render thread stopping.");
    };

private:
    graphics::ApplicationGlfw & mApplication;

    std::queue<std::function<void()>> mOperations;
    std::mutex mOperationsMutex;

    std::atomic<bool> mThrew{false};
    std::exception_ptr mThreadException;

    std::atomic<bool> mStop{false};
    std::thread mThread;
};


void runApplication()
{
    SELOG(info)("I'm a snac man.");
    SELOG(debug)("Initial delta time {}ms.", std::chrono::duration_cast<ms>(gSimulationDelta).count());

    // Application and window initialization
    graphics::ApplicationGlfw glfwApp{getVersionedName(),
                                      800, 600 // TODO handle via settings
                                      //TODO, applicationFlags
    };
    // Context must be removed from this thread before it can be made current on the render thread.
    glfwApp.removeCurrentContext();

    //
    // Initialize scene
    //
    bawls::Bawls scene{*glfwApp.getAppInterface()};

    //
    // Initialize rendering subsystem
    //
    GraphicStateFifo graphicStates;
    RenderThread renderingThread{glfwApp,
                                 graphicStates,
                                 scene.getWindowWorldSize()};

    auto viewportListening = glfwApp.getAppInterface()->listenFramebufferResize(
        std::bind(&RenderThread::resizeViewport, &renderingThread, std::placeholders::_1));


    //
    // Main simulation loop
    //
    Clock::time_point endStepTime = Clock::now();
    while(glfwApp.handleEvents())
    {
        Clock::time_point beginStepTime = endStepTime;

        // Regularly check if the rendering thread did not throw
        renderingThread.checkRethrow();

        Clock::duration simulationDelta = gImguiGameLoop.getSimulationDelta();

        //
        // Release CPU cycles until next time point to advance the simulation.
        //
        if (!gWaitByBusyLoop)
        {
            if (auto beforeSleep = Clock::now(); (beginStepTime + simulationDelta) > beforeSleep)
            {
                Clock::duration ahead = beginStepTime + simulationDelta - beforeSleep;
                // Could be sleep_until
                std::this_thread::sleep_for(ahead);

                SELOG(trace)("Expected to sleep for {}ms, actually slept for {}ms",
                        std::chrono::duration_cast<ms>(ahead).count(),
                        std::chrono::duration_cast<ms>(Clock::now() - beforeSleep).count());
            }
        }
        else
        {
            auto beforeSleep = Clock::now();
            Clock::duration ahead = beginStepTime + simulationDelta - beforeSleep;
            while((beginStepTime + simulationDelta) > Clock::now())
            {
                std::this_thread::sleep_for(Clock::duration{0});
            }
            SELOG(trace)("Expected to sleep for {}ms, actually slept for {}ms",
                    std::chrono::duration_cast<ms>(ahead).count(),
                    std::chrono::duration_cast<ms>(Clock::now() - beforeSleep).count());
        }

        //
        // Simulate one step
        //
        scene.update((float)asSeconds(simulationDelta));

        //
        // Push the graphic state for the latest simulated state
        //
        graphicStates.push(scene.makeGraphicState());

        endStepTime = Clock::now();
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
