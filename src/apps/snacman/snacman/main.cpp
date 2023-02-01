#include "GraphicState.h"
#include "Logging.h"
#include "simulations/cubes/Cubes.h"
#include "simulations/snacgame/SnacGame.h"
#include "snacman/LoopSettings.h"
#include "Timing.h"

#include <build_info.h>

// TODO we should not include something from detail.
// So either move it out of detail, either use nholmann directly
#include <arte/detail/Json.h>

#include <graphics/ApplicationGlfw.h>
#include <imguiui/ImguiUi.h>
#include <math/VectorUtilities.h>

#include <platform/Path.h>

#include <resource/ResourceFinder.h>

#include <queue>
#include <thread>

using namespace ad;
using namespace ad::snac;

// #define BAWLS_SCENE
#define SNAC_SCENE

#if defined(SNAC_SCENE)
using Simu_t = snacgame::SnacGame;
#endif

using GraphicStateFifo = StateFifo<Simu_t::Renderer_t::GraphicState_t>;

constexpr bool gWaitByBusyLoop = true;
// TODO find a better place than global

/// \brief A double buffer implementation, consuming entries from a
/// GraphicStateFifo.
class EntryBuffer
{
public:
    /// \brief Construct the buffer, block until it could initialize all its
    /// entries.
    EntryBuffer(GraphicStateFifo & aStateFifo)
    {
        for (std::size_t i = 0; i != std::size(mDoubleBuffer); ++i)
        {
            // busy wait
            while (aStateFifo.empty())
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
        while (!aStateFifo.empty())
        {
            mFront = back();
            mDoubleBuffer[mFront] = aStateFifo.pop();
            SELOG(trace)("Render thread: Newer state retrieved.");
        }
    }

private:
    std::size_t back() const { return (mFront + 1) % 2; }

    std::array<GraphicStateFifo::Entry, 2> mDoubleBuffer;
    std::size_t mFront = 1;
};

template <class T_renderer>
class RenderThread
{
    using Operation = std::function<void(T_renderer &)>;

public:
    RenderThread(graphics::ApplicationGlfw & aGlfwApp,
                 GraphicStateFifo & aStates,
                 T_renderer && aRenderer,
                 imguiui::ImguiUi & aImguiUi,
                 std::atomic<bool> & aInterpolate) :
        mApplication{aGlfwApp},
        mViewportListening{
            aGlfwApp.getAppInterface()->listenFramebufferResize(std::bind(
                &RenderThread::resizeViewport, this, std::placeholders::_1))},
        mImguiUi{aImguiUi},
        mInterpolate{aInterpolate}
    {
        mThread = std::thread{
            [this, &aStates, renderer = std::move(aRenderer)]() mutable {
                // Note: the move below is why we cannot use std::bind.
                // We know that the lambda is invoked only once, so we can move
                // from its closure.
                run(aStates, std::move(renderer));
            }};
    }

    /// \brief Stop the thread and rethrow if it actually threw.
    ///
    /// A destructor should never throw, so we cannot implement this behaviour
    /// in the destructor.
    void finalize()
    {
        stop();
        checkRethrow();
    }

    void resizeViewport(math::Size<2, int> aFramebufferSize)
    {
        push([=](T_renderer & /*aRenderer*/) {
            glViewport(0, 0, aFramebufferSize.width(),
                       aFramebufferSize.height());
        });
    }

    void resetProjection(float aAspectRatio,
                         snac::Camera::Parameters aParameters)
    {
        push([=](T_renderer & aRenderer) {
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
    void push(Operation aOperation)
    {
        std::lock_guard lock{mOperationsMutex};
        mOperations.push(std::move(aOperation));
    }

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
        catch (...)
        {
            mThreadException = std::current_exception();
            mThrew = true;
        }
    }

    void run_impl(GraphicStateFifo & aStates, T_renderer && aRenderer)
    {
        SELOG(info)("Render thread started");

        // The context must be made current on this thread before it can call GL
        // functions.
        mApplication.makeContextCurrent();

        // At first, Renderer was constructed here directly, in the render
        // thread, because the ctor makes OpenGL calls (and the GL context is
        // current on the render thread).
        // T_renderer renderer{aWindowSize_world};

        // Yet, I am afraid it would be limiting, so it could either:
        // * forward variadic ctor args
        // * forward a factory function
        // * move a fully constructed Renderer here (constructed in main thread,
        // before releasing GL context) We currently use the 3rd approach.
        // Important: move into a **local copy**, so dtor is called on the
        // render thread, where GL context is active.
        T_renderer renderer{std::move(aRenderer)};

        // Used by non-interpolating path, to decide if frame is dirty.
        Clock::time_point renderedPushTime;

        // Initialize the buffer with the required number of entries (2 for
        // double buffering) This is blocking call, done last to offer more
        // opportunities for the entries to already be in the queue
        EntryBuffer entries{aStates};
        SELOG(info)("Render thread initialized states buffer.");

        while (!mStop)
        {
            // Service all queued operations first.
            // The implementation takes care not to hold onto the mutex
            // while executing the operation.
            {
                while (true /* explicit break in body */)
                {
                    Operation operation;
                    {
                        std::lock_guard lock{mOperationsMutex};
                        if (mOperations.empty())
                        {
                            // Breaks the service operations loop.
                            break;
                        }
                        else
                        {
                            // Moves the operation out of the queue (and pop it).
                            operation = std::move(mOperations.front());
                            mOperations.pop();
                        }
                    }
                    // Execute the operations while the mutex is unlocked.
                    operation(renderer);
                }
            }

            // TODO simulate delay in the render thread:
            // * Thread iteration time (simulate what CPU compuations run on the
            // thread, e.g. visibility).
            // * GPU load, i.e. rendering time. This might be harder to
            // simulate.
            // std::this_thread::sleep_for(ms{8});

            // Get new latest state, if any
            entries.consume(aStates);

            //
            // Interpolate (or pick the current state if interpolation is
            // disabled)
            //
            typename T_renderer::GraphicState_t state;
            if (mInterpolate)
            {
                const auto & previous = entries.previous();
                const auto & latest = entries.current();

                // TODO Is it better to use difference in push times, or the
                // fixed simulation delta as denominator? Note: using the
                // difference in push time smoothly "slows" the game if push
                // occurs less frequently than the simulation period would
                // require.
                float interpolant =
                    float((Clock::now() - latest.pushTime).count())
                    / (latest.pushTime - previous.pushTime).count();
                SELOG(trace)
                ("Render thread: Interpolant is {}, delta between entries is "
                 "{}ms.",
                 interpolant,
                 duration_cast<ms>(latest.pushTime - previous.pushTime)
                     .count());

                state =
                    interpolate(*previous.state, *latest.state, interpolant);
            }
            else
            {
                const auto & latest = entries.current();
                if (latest.pushTime != renderedPushTime)
                {
                    state = *latest.state;
                    renderedPushTime = latest.pushTime;
                }
                else
                {
                    // The last frame rendered is still for the latest state,
                    // non need to render.
                    continue;
                }
            }

            //
            // Render
            //
            mApplication.getAppInterface()->clear();
            // renderer.render(*entries.current().state);
            renderer.render(state);
            SELOG(trace)("Render thread: Frame sent to GPU.");

            mImguiUi.renderBackend();

            mApplication.swapBuffers();
        }

        SELOG(info)("Render thread stopping.");
    };

private:
    graphics::ApplicationGlfw & mApplication;
    std::shared_ptr<graphics::AppInterface::SizeListener> mViewportListening;

    std::queue<Operation> mOperations;
    std::mutex mOperationsMutex;
    imguiui::ImguiUi & mImguiUi;

    std::atomic<bool> & mInterpolate;

    std::atomic<bool> mThrew{false};
    std::exception_ptr mThreadException;

    std::atomic<bool> mStop{false};
    std::thread mThread;
};


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

    // Application and window initialization
    graphics::ApplicationGlfw glfwApp{
        getVersionedName(), 800, 600 // TODO handle via settings
                                     // TODO, applicationFlags
    };

    imguiui::ImguiUi imguiUi(glfwApp);

    resource::ResourceFinder resourceFinder = makeResourceFinder();

    ConfigurableSettings configurableSettings;

    //
    // Initialize input devices
    //
    HidManager hid{glfwApp};
    RawInput input = hid.initialInput();
    snacgame::ImguiInhibiter inhibiter;

    //
    // Initialize scene
    //
    Simu_t scene{*glfwApp.getAppInterface(), imguiUi, resourceFinder, input};

    //
    // Initialize rendering subsystem
    //

    // Initialize the renderer
    Simu_t::Renderer_t renderer = scene.makeRenderer(resourceFinder);

    // Context must be removed from this thread before it can be made current on
    // the render thread.
    glfwApp.removeCurrentContext();

    GraphicStateFifo graphicStates;
    RenderThread renderingThread{glfwApp, graphicStates, std::move(renderer),
                                 imguiUi, configurableSettings.mInterpolate};

#if defined(CUBE_SCENE)
    // Reset the camera projection when the window size changes
    auto mWindowSizeListening = glfwApp.getAppInterface()->listenWindowResize(
        [&renderingThread, &scene](math::Size<2, int> aWindowSize) {
            renderingThread.resetProjection(math::getRatio<float>(aWindowSize),
                                            scene.getCameraParameters());
        });
#endif

    //
    // Main simulation loop
    //
    Clock::time_point beginStepTime =
        Clock::now() - configurableSettings.mSimulationDelta;

    while (glfwApp.handleEvents())
    {
        // Update input
        input = hid.read(input, inhibiter);

        //
        // Simulate one step
        //
        if (scene.update(
                (float) asSeconds(configurableSettings.mSimulationDelta),
                input))
        {
            break;
        }

        scene.drawDebugUi(configurableSettings, inhibiter, input);
        // Pretend update took longer if requested by user.
        sleepBusy(configurableSettings.mUpdateDuration, beginStepTime);

        //
        // Push the graphic state for the latest simulated state
        //
        graphicStates.push(scene.makeGraphicState());

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
