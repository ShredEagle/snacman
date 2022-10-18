#include "Logging.h"
#include "Timing.h"
#include "GraphicState.h"

#include <build_info.h>

#include <graphics/ApplicationGlfw.h>


using namespace ad;
using namespace ad::snac;


using ms = std::chrono::milliseconds;
//
//constexpr Clock::duration gSimulationDelta = 1.f/60;
constexpr Clock::duration gSimulationDelta = ms{50};

constexpr bool gWaitByBusyLoop = true;


class RenderThread
{
public:
    RenderThread(graphics::ApplicationGlfw & aGlfwApp,
                 StateFifo<GraphicState> & aStates) :
        mApplication{aGlfwApp}
    {
        mThread = std::thread{
            std::bind(&RenderThread::run,
                      this,
                      std::ref(aStates))};
    }

    ~RenderThread()
    {
        mStop = true;
        mThread.join();
    }

private:
    void run(StateFifo<GraphicState> & aStates)
    {
        SELOG(info)("Render thread started");

        // The context must be made current on this thread before it can call GL functions.
        mApplication.makeContextCurrent();

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

        while(!mStop)
        {
            std::this_thread::sleep_for(ms{8});

            // TODO interpolate
            mApplication.getAppInterface()->clear();
            // TODO render
            SELOG(trace)("Render thread: Done (not) rendering.");
            mApplication.swapBuffers();

            if (auto latest = aStates.popToEnd(); latest)
            {
                entry = std::move(*latest);
                SELOG(trace)("Render thread: Newer state retrieved.");
            }
        }

        SELOG(info)("Render thread stopping.");
    };

private:
    std::atomic<bool> mStop{false};
    graphics::ApplicationGlfw & mApplication;
    std::thread mThread;
};


int main(int argc, char * argv[])
{
    snac::detail::initializeLogging();
    spdlog::set_level(spdlog::level::trace);

    SELOG(info)("I'm a snac man.");
    SELOG(debug)("Delta time {}ms.", std::chrono::duration_cast<ms>(gSimulationDelta).count());

    // Application and window initialization
    graphics::ApplicationGlfw glfwApp{getVersionedName(),
                                      800, 600 // TODO handle via settings
                                      //TODO, applicationFlags
    };
    // Context must be removed from this thread before it can be made current on the render thread.
    glfwApp.removeCurrentContext();

    StateFifo<GraphicState> graphicStates;
    RenderThread renderingThread{glfwApp, graphicStates};

    //
    // TODO Initialize scene
    //

    Clock::time_point endStepTime = Clock::now();

    while(glfwApp.handleEvents())
    {
        Clock::time_point beginStepTime = endStepTime;

        //
        // Release CPU cycles until next time point to advance the simulation.
        //
        if (!gWaitByBusyLoop)
        {
            if (auto beforeSleep = Clock::now(); (beginStepTime + gSimulationDelta) > beforeSleep)
            {
                Clock::duration ahead = beginStepTime + gSimulationDelta - beforeSleep;
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
            Clock::duration ahead = beginStepTime + gSimulationDelta - beforeSleep;
            while((beginStepTime + gSimulationDelta) > Clock::now())
            {
                std::this_thread::sleep_for(Clock::duration{0});
            }
            SELOG(trace)("Expected to sleep for {}ms, actually slept for {}ms",
                    std::chrono::duration_cast<ms>(ahead).count(),
                    std::chrono::duration_cast<ms>(Clock::now() - beforeSleep).count());
        }

        //
        // TODO simulate one step
        //

        graphicStates.push(std::make_unique<GraphicState>());

        endStepTime = Clock::now();
    }

    return 0;
}
