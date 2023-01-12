#include "Logging.h"
#include "Timing.h"
#include "GraphicState.h"

#include "simulations/cubes/Cubes.h"
#include "simulations/snacgame/SnacGame.h"

#include <build_info.h>

#include <graphics/ApplicationGlfw.h>

#include <imguiui/ImguiUi.h>

#include <math/VectorUtilities.h>

#include <queue>
#include <thread>


using namespace ad;
using namespace ad::snac;


//#define BAWLS_SCENE
#define SNAC_SCENE

#if defined(CUBE_SCENE)
using Simu_t = cubes::Cubes;
#elif defined(SNAC_SCENE)
using Simu_t = snacgame::SnacGame;
#endif

using GraphicStateFifo = StateFifo<Simu_t::Renderer_t::GraphicState_t>;


using ms = std::chrono::milliseconds;

constexpr Clock::duration gSimulationDelta = Clock::duration{std::chrono::seconds{1}} / 60;
//constexpr Clock::duration gSimulationDelta = ms{50};

constexpr bool gWaitByBusyLoop = true;


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

        int updateDuration = (int)mUpdateDuration;
        if(ImGui::InputInt("Update duration (ms)", &updateDuration))
        {
            updateDuration = std::clamp(updateDuration, 1, 500);
        }
        mUpdateDuration = updateDuration;

        ImGui::Checkbox("State interpolation", &mInterpolate);
    }

    Clock::duration getSimulationDelta() const
    {
        return Clock::duration{ms{mSimulationPeriodMs}};
    }

    /// \brief The (minimal) duration that **computing** an update should consume.
    /// \attention Do not confound with the simulation delta, which is the duration between two simulated states.
    Clock::duration getUpdateDuration()
    {
        return Clock::duration{ms{mUpdateDuration}};
    }

    bool isInterpoling() const
    {
        return mInterpolate;
    }

    enum WantCapture
    {
        Null,
        Mouse = 1 << 0,
        Keyboard = 1 << 1 ,
    };

    // TODO use a flag type
    void resetCapture(WantCapture aCaptures)
    {
        mCaptures = aCaptures;
    }

    bool isCapturing(WantCapture aCapture)
    {
        return (mCaptures & aCapture) == aCapture;
    }

private:
    std::atomic<Clock::duration::rep> mSimulationPeriodMs{duration_cast<ms>(gSimulationDelta).count()};
    std::atomic<Clock::duration::rep> mUpdateDuration{0};

    std::atomic<std::uint8_t> mCaptures{0};

    // Synchronous part (only seen by the rendering thread)
    bool mInterpolate{true};
};


// TODO find a better place than global
ImguiGameLoop gImguiGameLoop;


/// \brief Implement HidManager's Inhibiter protocol, for Imgui.
/// Allowing the app to discard input events that are handled by DearImgui.
class ImguiInhibiter : public HidManager::Inhibiter
{
    bool isCapturingMouse() const override
    { return gImguiGameLoop.isCapturing(ImguiGameLoop::Mouse); }
    bool isCapturingKeyboard() const override
    { return gImguiGameLoop.isCapturing(ImguiGameLoop::Keyboard); }
};


/// \brief A double buffer implementation, consuming entries from a GraphicStateFifo.
class EntryBuffer
{
public:
    /// \brief Construct the buffer, block until it could initialize all its entries.
    EntryBuffer(GraphicStateFifo & aStateFifo)
    {
        for (std::size_t i = 0; i != std::size(mDoubleBuffer); ++i)
        {
            // busy wait
            while(aStateFifo.empty())
            {}

            mDoubleBuffer[i] = aStateFifo.pop();
        }

        // Make sure the EntryBuffer is up-to-date
        consume(aStateFifo);
    }

    const GraphicStateFifo::Entry & current() const
    {
        return mDoubleBuffer[mFront];
    }

    const GraphicStateFifo::Entry & previous() const
    {
        return mDoubleBuffer[back()];
    }

    /// \brief Pop as many states as available from the Fifo.
    ///
    /// Ensures the buffer current() and previous() entries are
    /// up-to-date at the time of return (modulo return branching duration).
    void consume(GraphicStateFifo & aStateFifo)
    {
        while(!aStateFifo.empty())
        {
            mFront = back();
            mDoubleBuffer[mFront] = aStateFifo.pop();
            SELOG(trace)("Render thread: Newer state retrieved.");
        }
    }

private:
    std::size_t back() const
    {
        return (mFront + 1) % 2;
    }

    std::array<GraphicStateFifo::Entry, 2> mDoubleBuffer;
    std::size_t mFront = 1;
};

template <class T_renderer>
class RenderThread
{
public:
    RenderThread(graphics::ApplicationGlfw & aGlfwApp,
                 GraphicStateFifo & aStates,
                 T_renderer && aRenderer) :
        mApplication{aGlfwApp},
        mViewportListening{
            aGlfwApp.getAppInterface()->listenFramebufferResize(
                std::bind(&RenderThread::resizeViewport, this, std::placeholders::_1))}
    {
        mThread = std::thread{[this, &aStates, renderer=std::move(aRenderer)]() mutable
            {
                // Note: the move below is why we cannot use std::bind.
                // We know that the lambda is invoked only once, so we can move from its closure.
                run(aStates, std::move(renderer));
            }};

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
        mOperations.push([=](T_renderer & /*aRenderer*/)
            {
                glViewport(0, 0, aFramebufferSize.width(), aFramebufferSize.height());
            });
    }

    void resetProjection(float aAspectRatio, snac::Camera::Parameters aParameters)
    {
        std::lock_guard lock{mOperationsMutex};
        mOperations.push([=](T_renderer & aRenderer)
            {
                aRenderer.resetProjection(aAspectRatio, aParameters);
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

    void run(GraphicStateFifo & aStates, T_renderer && aRenderer)
    {
        try
        {
            run_impl(aStates, std::move(aRenderer));
        }
        catch(...)
        {
            mThreadException = std::current_exception();
            mThrew = true;
        }
    }

    void run_impl(GraphicStateFifo & aStates, T_renderer && aRenderer)
    {
        SELOG(info)("Render thread started");

        // The context must be made current on this thread before it can call GL functions.
        mApplication.makeContextCurrent();

        //
        // Imgui
        //
        imguiui::ImguiUi ui{mApplication};
        bool showImguiDemo = true;

        // At first, Renderer was constructed here directly, in the render thread, because the ctor makes
        // OpenGL calls (and the GL context is current on the render thread).
        //T_renderer renderer{aWindowSize_world};

        // Yet, I am afraid it would be limiting, so it could either:
        // * forward variadic ctor args
        // * forward a factory function
        // * move a fully constructed Renderer here (constructed in main thread, before releasing GL context)
        // We currently use the 3rd approach. 
        // Important: move into a **local copy**, so dtor is called on the render thread, where GL context is active.
         T_renderer renderer{std::move(aRenderer)};

        // Used by non-interpolating path, to decide if frame is dirty.
        Clock::time_point renderedPushTime;

        // Initialize the buffer with the required number of entries (2 for double buffering)
        // This is blocking call, done last to offer more opportunities for the entries to already be in the queue
        EntryBuffer entries{aStates};
        SELOG(info)("Render thread initialized states buffer.");

        while(!mStop)
        {
            // Service all queued operations first
            {
                std::lock_guard lock{mOperationsMutex};
                while(!mOperations.empty())
                {
                    // Execute operation then pop it.
                    mOperations.front()(renderer); 
                    mOperations.pop();
                }
            }

            // TODO simulate delay in the render thread:
            // * Thread iteration time (simulate what CPU compuations run on the thread, e.g. visibility).
            // * GPU load, i.e. rendering time. This might be harder to simulate.
            //std::this_thread::sleep_for(ms{8});

            // Get new latest state, if any
            entries.consume(aStates);

            //
            // Interpolate (or pick the current state if interpolation is disabled)
            //
            typename T_renderer::GraphicState_t state;
            if(gImguiGameLoop.isInterpoling())
            {
                const auto & previous = entries.previous();
                const auto & latest = entries.current();

                // TODO Is it better to use difference in push times, or the fixed simulation delta as denominator?
                // Note: using the difference in push time smoothly "slows" the game if push
                // occurs less frequently than the simulation period would require.
                float interpolant = float((Clock::now() - latest.pushTime).count())
                                    / (latest.pushTime - previous.pushTime).count();
                SELOG(trace)("Render thread: Interpolant is {}, delta between entries is {}ms.",
                    interpolant,
                    duration_cast<ms>(latest.pushTime - previous.pushTime).count());

                state = interpolate(*previous.state, *latest.state, interpolant);
            }
            else
            {
                const auto & latest = entries.current();
                if(latest.pushTime != renderedPushTime)
                {
                    state = *latest.state;
                    renderedPushTime = latest.pushTime;
                }
                else
                {
                    // The last frame rendered is still for the latest state, non need to render.
                    continue;
                }
            }

            //
            // Render
            //
            mApplication.getAppInterface()->clear();
            //renderer.render(*entries.current().state);
            renderer.render(state);
            SELOG(trace)("Render thread: Frame sent to GPU.");

            // Imgui rendering
            ui.newFrame();
            // NewFrame() updates the io catpure flag: consume them ASAP
            // see: https://pixtur.github.io/mkdocs-for-imgui/site/FAQ/#qa-integration
            gImguiGameLoop.resetCapture(static_cast<ImguiGameLoop::WantCapture>(
                (ui.isCapturingMouse() ? ImguiGameLoop::Mouse : ImguiGameLoop::Null) 
                | (ui.isCapturingKeyboard() ? ImguiGameLoop::Keyboard : ImguiGameLoop::Null)));

            ImGui::ShowDemoWindow(&showImguiDemo);
            ImGui::Begin("Gameloop");
            //ImGui::Checkbox("Demo window", &showImguiDemo);
            gImguiGameLoop.render();
            ImGui::End();

            ui.render();

            mApplication.swapBuffers();
        }

        SELOG(info)("Render thread stopping.");
    };

private:
    graphics::ApplicationGlfw & mApplication;
    std::shared_ptr<graphics::AppInterface::SizeListener> mViewportListening;

    std::queue<std::function<void(T_renderer &)>> mOperations;
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

    //
    // Initialize scene
    //
    Simu_t scene{*glfwApp.getAppInterface()};

    //
    // Initialize rendering subsystem
    //

    // Initialize the renderer
    Simu_t::Renderer_t renderer = scene.makeRenderer();

    // Context must be removed from this thread before it can be made current on the render thread.
    glfwApp.removeCurrentContext();

    GraphicStateFifo graphicStates;
    RenderThread renderingThread{glfwApp,
                                 graphicStates,
                                 std::move(renderer)};

#if defined(CUBE_SCENE)
    // Reset the camera projection when the window size changes
    auto mWindowSizeListening = glfwApp.getAppInterface()->listenWindowResize(
        [&renderingThread, &scene](math::Size<2, int> aWindowSize)
        {
            renderingThread.resetProjection(math::getRatio<float>(aWindowSize), scene.getCameraParameters());
        });
#endif

    //
    // Initialize input devices
    //
    HidManager hid{glfwApp};
    Input input = hid.initialInput();
    ImguiInhibiter inhibiter;

    //
    // Main simulation loop
    //
    Clock::time_point beginStepTime = Clock::now() - gImguiGameLoop.getSimulationDelta();
    while(glfwApp.handleEvents())
    {
        Clock::time_point previousStepTime = beginStepTime;

        // Regularly check if the rendering thread did not throw
        renderingThread.checkRethrow();

        Clock::duration simulationDelta = gImguiGameLoop.getSimulationDelta();

        // Update input
        input = hid.read(input, inhibiter);

        //
        // Release CPU cycles until next time point to advance the simulation.
        //
        // TODO move, so the wait happend **before** handle events
        if (!gWaitByBusyLoop)
        {
            if (auto beforeSleep = Clock::now(); (previousStepTime + simulationDelta) > beforeSleep)
            {
                Clock::duration ahead = previousStepTime + simulationDelta - beforeSleep;
                // Could be sleep_until
                std::this_thread::sleep_for(ahead);

                SELOG(trace)("Expected to sleep for {}ms, actually slept for {}ms",
                        std::chrono::duration_cast<ms>(ahead).count(),
                        std::chrono::duration_cast<ms>(Clock::now() - beforeSleep).count());
            }
        }
        else
        {
            SleepResult slept = sleepBusy(simulationDelta, previousStepTime);
            SELOG(trace)("Expected to sleep for {}ms, actually slept for {}ms",
                    std::chrono::duration_cast<ms>(slept.targetDuration).count(),
                    std::chrono::duration_cast<ms>(Clock::now() - slept.timeBefore).count());
        }

        //
        // Simulate one step
        //
        beginStepTime = Clock::now();
        scene.update((float)asSeconds(simulationDelta), input);
        // Pretend update took longer if requested by user.
        sleepBusy(gImguiGameLoop.getUpdateDuration(), beginStepTime);

        //
        // Push the graphic state for the latest simulated state
        //
        graphicStates.push(scene.makeGraphicState());
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
