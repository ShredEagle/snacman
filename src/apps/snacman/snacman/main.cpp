#include "Logging.h"
#include "Timing.h"
#include "GraphicState.h"

#include <build_info.h>

#include <graphics/ApplicationGlfw.h>

#include <queue>


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
        stop();
    }

    /// \brief Stop the thread and rethrow if it actually threw.
    ///
    /// A destructor should never throw, so we cannot rethrow in the plain destructor. 
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

    void run(StateFifo<GraphicState> & aStates)
    {
        try
        {
            run_impl(aStates);
        }
        catch(...)
        {
            mThreadException = std::current_exception();
            mThrew = true;
        }
    }

    void run_impl(StateFifo<GraphicState> & aStates)
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
            // Service all queued operations first
            {
                std::lock_guard lock{mOperationsMutex};
                while(!mOperations.empty())
                {
                    mOperations.front()();
                    mOperations.pop();
                }
            }

            std::this_thread::sleep_for(ms{8});

            // TODO interpolate

            mApplication.getAppInterface()->clear();
            // TODO render
            SELOG(trace)("Render thread: Done (not) rendering.");
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

    auto viewportListening = glfwApp.getAppInterface()->listenFramebufferResize(
        std::bind(&RenderThread::resizeViewport, &renderingThread, std::placeholders::_1));

    //
    // TODO Initialize scene
    //

    Clock::time_point endStepTime = Clock::now();

    while(glfwApp.handleEvents())
    {
        Clock::time_point beginStepTime = endStepTime;

        // Regularly check if the rendering thread did not throw
        renderingThread.checkRethrow();

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

    // Optional, but guarantees that we did not miss any excpetion occuring between the last check
    // and the thread destruction.
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
