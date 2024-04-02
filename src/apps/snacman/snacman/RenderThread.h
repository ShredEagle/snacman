#pragma once


#include "GraphicState.h"
#include "Logging.h"
#include "LoopSettings.h"
#include "Profiling.h"
#include "Profiling_V2.h"
#include "ProfilingGPU.h"
#include "Timing.h"

#include <graphics/ApplicationGlfw.h>

#include <imguiui/ImguiUi.h>

#include <resource/ResourceFinder.h>

// TODO Ad 2024/02/14: #RV2 Remove V1 includes
#include <snac-renderer-V1/Camera.h>
#include <snac-renderer-V1/Mesh.h>
#include <snac-renderer-V1/text/Text.h>

#include <snac-renderer-V2/Model.h>

#include <future>
#include <queue>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <processthreadsapi.h>
#undef WIN32_LEAN_AND_MEAN 
#endif

namespace ad {
namespace snac {


class Resources;


// Compilation firewall: required because Resources include RenderThread.h,
// so we cannot access Resources defintion from the header.
void recompilePrograms(Resources & aResources);


template <class T_renderer>
using GraphicStateFifo = StateFifo<typename T_renderer::GraphicState_t>;


/// \brief A double buffer implementation, consuming entries from a
/// GraphicStateFifo.
template <class T_renderer>
class EntryBuffer
{
    using GraphicStateFifo_t = GraphicStateFifo<T_renderer>;

public:
    static constexpr std::size_t BufferDepth = 2;
    static_assert(BufferDepth == 2, "Current implementation is limited to double buffering.");

    /// \brief Construct the buffer, block until it could initialize all its
    /// entries.
    EntryBuffer(GraphicStateFifo_t & aStateFifo)
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

    const typename GraphicStateFifo_t::Entry & current() const
    {
        return mDoubleBuffer[mFront];
    }

    const typename GraphicStateFifo_t::Entry & previous() const
    {
        return mDoubleBuffer[back()];
    }

    /// \brief Pop as many states as available from the Fifo.
    ///
    /// Ensures the buffer current() and previous() entries are
    /// up-to-date at the time of return (modulo return branching duration).
    void consume(GraphicStateFifo_t & aStateFifo)
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

    std::array<typename GraphicStateFifo_t::Entry, BufferDepth> mDoubleBuffer;
    std::size_t mFront = 1;
};

template <class T_renderer>
class RenderThread
{
    using GraphicStateFifo_t = GraphicStateFifo<T_renderer>;
    using Operation = std::function<void(T_renderer &)>;

    struct Controls
    {
        std::atomic<bool> & mInterpolate;
    };

public:
    RenderThread(graphics::ApplicationGlfw & aGlfwApp,
                 GraphicStateFifo_t & aStates,
                 T_renderer && aRenderer,
                 imguiui::ImguiUi & aImguiUi,
                 ConfigurableSettings & aSettingsIncludingControls) :
        mApplication{aGlfwApp},
        mViewportListening{
            aGlfwApp.getAppInterface()->listenFramebufferResize(std::bind(
                &RenderThread::resizeViewport, this, std::placeholders::_1))},
        mRenderer{std::move(aRenderer)},
        mImguiUi{aImguiUi},
        mControls{
            .mInterpolate = aSettingsIncludingControls.mInterpolate,
        }
    {
        mThread = std::thread{
            [this, &aStates]() mutable {
                run(aStates);
            }};
    }

    // The destructor is stopping the thread, otherwise if an exception occurs
    // in the main thread, and stack unwinding destroys the render thread,
    // terminate would be called (thus missing the top-level catch in main)
    ~RenderThread()
    {
        if(mThread.joinable())
        {
            SELOG(warn)("Destructor stopping render thread which was not finalized.");
            stop();
        }
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

    //void resetProjection(float aAspectRatio,
    //                     snac::Camera::Parameters aParameters)
    //{
    //    push([=](T_renderer & aRenderer) {
    //        aRenderer.resetProjection(aAspectRatio, aParameters);
    //    });
    //}

    std::future<typename T_renderer::template Handle_t<const renderer::Object>> 
    loadModel(filesystem::path aModel, 
              filesystem::path aEffect, 
              typename T_renderer::Resources_t & aResources)
    {
        // Almost certainly a programming error:
        // There is a risk the calling code will block on the future completion
        // If the render thread is blocking, the completion will never occur.
        assert(std::this_thread::get_id() != mThread.get_id());

        // std::function require the type-erased functor to be copy constructible.
        // all captured types must be copyable.
        using Handle_t = typename T_renderer::template Handle_t<const renderer::Object>;
        auto promise = std::make_shared<std::promise<Handle_t>>();
        std::future<Handle_t> future = promise->get_future();
        push([promise = std::move(promise), shape = std::move(aModel), effect = std::move(aEffect), &aResources]
             (T_renderer & aRenderer) 
             {
                try
                {
                    promise->set_value(aRenderer.loadModel(shape, effect, aResources));
                }
                catch(...)
                {
                    promise->set_exception(std::current_exception());
                }
             });
        return future;
    }

    std::future<std::shared_ptr<snac::Font>> loadFont(
        arte::FontFace aFontFace,
        unsigned int aPixelHeight,
        filesystem::path aEffect,
        Resources & aResources)
    {
        assert(std::this_thread::get_id() != mThread.get_id());

        // std::function require the type-erased functor to be copy constructible.
        // all captured types must be copyable.
        auto promise = std::make_shared<std::promise<std::shared_ptr<snac::Font>>>();
        std::future<std::shared_ptr<snac::Font>> future = promise->get_future();
        // TODO the Text system has to be deeply refactored. It is tightly coupled to the texture creation
        // making it hard to properly load asynchronously, and leading to horrors such as this move into shared_ptr
        auto sharedFontFace = std::make_shared<arte::FontFace>(std::move(aFontFace));
        push([promise = std::move(promise),
              fontFace = std::move(sharedFontFace),
              aPixelHeight,
              effect = std::move(aEffect),
              &aResources]
             (T_renderer & aRenderer) mutable
             {
                try
                {
                    promise->set_value(aRenderer.loadFont(std::move(*fontFace), aPixelHeight, effect, aResources));
                }
                catch(...)
                {
                    promise->set_exception(std::current_exception());
                }
             });
        return future;
    }


    std::future<void> recompileShaders(Resources & aResources)
    {
        assert(std::this_thread::get_id() != mThread.get_id());

        // std::function require the type-erased functor to be copy constructible.
        // all captured types must be copyable.
        auto promise = std::make_shared<std::promise<void>>();
        std::future<void> future = promise->get_future();
        push([promise = std::move(promise), &aResources]
             (T_renderer & aRenderer) mutable
             {
                try
                {
                    recompilePrograms(aResources);
                    // Required because the actual interface of the shader might have changed
                    // (either explicitly by code, or implicitly because optimizer discarded some attributes/uniforms)
                    aRenderer.resetRepositories();
                    promise->set_value();
                }
                catch(...)
                {
                    promise->set_exception(std::current_exception());
                }
             });
        return future;
    }

    void continueGui()
    {
        assert(!mStop); // It is unsafe to call this while the render thread might be (explicitly) destructing the Renderer.
        mRenderer->continueGui();
    }

    void checkRethrow()
    {
        // Should check from another thread, not the render thread.
        assert(std::this_thread::get_id() != mThread.get_id());

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

    void run(GraphicStateFifo_t & aStates)
    {
        try
        {
            run_impl(aStates);
        }
        catch (...)
        {
            try
            {
                std::rethrow_exception(std::current_exception());
            }
            catch(const std::exception & e)
            {
                SELOG(error)
                    ("Render thread main loop stopping due to uncaught exception:\n{}",
                    e.what());
            }
            catch(...)
            {
                SELOG(critical)("Render thread main loop stopping after throwing a non-std::exception.");
            }
            mThreadException = std::current_exception();
            mThrew = true;
        }
    }
            
    void serviceOperations(T_renderer & aRenderer)
    {
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
                operation(aRenderer);
            }
        }
    }

    void run_impl(GraphicStateFifo_t & aStates)
    {
        SELOG(info)("Render thread started");

// TODO Ad 2024/02/13: Abstract thread naming for all platforms in platform lib.
#if defined(_WIN32)
        HRESULT r;
        r = SetThreadDescription(GetCurrentThread(), L"Render Thread");
#endif

        // The context must be made current on this thread before it can call GL
        // functions.
        mApplication.makeContextCurrent();


        // Used by non-interpolating path, to decide if frame is dirty.
        Clock::time_point renderedPushTime;

        // If the client is waiting on a future from this thread 
        // before producing the two first states, we must service to avoid a deadlock.
        std::optional<EntryBuffer<T_renderer>> entries;
        while(!entries && !mStop)
        {
            // Note: this is busy looping at the moment.
            // This should only be for a brief period at the beginning.
            serviceOperations(*mRenderer);

            if(aStates.size() >= EntryBuffer<T_renderer>::BufferDepth)
            {
                // Initialize the buffer with the required number of entries (2 for
                // double buffering) This is blocking call, done last to offer more
                // opportunities for the entries to already be in the queue
                entries = EntryBuffer<T_renderer>{aStates};
                SELOG(info)("Render thread initialized states buffer.");
            }
        }

        while (!mStop)
        {
            // The V1 profiler does not require the frame ot be ended before writting its output.
            Guard frameProfiling = profileFrame(getProfilerGL());

            // NOTE: The artificial scope below is to help with profiler V2
            // At the moment, we must end the frame before calling the print to ostream
            {
                Guard frameProfiling_v2 = renderer::scopeProfilerFrame(renderer::gRenderProfiler);

                // Without the swap buffer
                auto iterationProfileEntry = BEGIN_RECURRING_GL("Iteration");

                // Service all queued operations first.
                {
                    TIME_RECURRING(Render, "Service_operations");
                    serviceOperations(*mRenderer);
                }

                // TODO simulate delay in the render thread:
                // * Thread iteration time (simulate what CPU compuations run on the
                // thread, e.g. visibility).
                // * GPU load, i.e. rendering time. This might be harder to
                // simulate.
                // std::this_thread::sleep_for(ms{8});

                // Get new latest state, if any
                {
                    TIME_RECURRING(Render, "Consume_available_states");
                    entries->consume(aStates);
                }

                //
                // Interpolate (or pick the current state if interpolation is
                // disabled)
                //
                typename T_renderer::GraphicState_t state;
                if (mControls.mInterpolate)
                {
                    TIME_RECURRING(Render, "Interpolation");

                    const auto & previous = entries->previous();
                    const auto & latest = entries->current();

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
                    const auto & latest = entries->current();
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
                auto frameProfileEntry = BEGIN_RECURRING_GL("Frame");
                    
                mApplication.getAppInterface()->clear();

                mRenderer->render(state);
                SELOG(trace)("Render thread: Frame sent to GPU.");

                {
                    TIME_RECURRING_GL( "ImGui::renderBackend");
                    mImguiUi.renderBackend();
                }

                END_RECURRING_GL(frameProfileEntry);
                END_RECURRING_GL(iterationProfileEntry);

                {
                    TIME_RECURRING_GL("Swap buffers");
                    mApplication.swapBuffers();
                }
            }

            // Now that the profiler V2 frame scope has ended, we can print its results. 
            {
                TIME_RECURRING_V1(Render, "RenderThread_Profiler_dump");
                getRenderProfilerPrint().print();
                v2::getRenderProfilerPrint().print();
            }
        }

        // This is smelly, but a consequence of the need to access it from both main and render thread
        // while the destruction should occur on the render thread (where the GL context is)
        mRenderer.reset(); // destruct on clean exit.
        SELOG(info)("Render thread stopping.");
    };

private:
    graphics::ApplicationGlfw & mApplication;
    std::shared_ptr<graphics::AppInterface::SizeListener> mViewportListening;

    // At first, Renderer was constructed in the run_impl scope, running on the render
    // thread, because the ctor makes OpenGL calls (and the GL context is
    // current on the render thread).
    // Yet, it was limiting, so we could either:
    // * forward variadic ctor args
    // * forward a factory function
    // * move an already constructed Renderer (constructed in main thread, before it releases GL context)
    // We currently use the 3rd approach.
    // Important: It was moved into a **local copy** scoped to run_impl, so dtor is called on the
    // render thread, where GL context is active.
    // This ensured that even in the case of exception cleaning occured on the RenderThread.
    // Yet, this conflicts with the need to call the drawGui method from the main thread.
    // This method needs to call drawGui() on the Renderer, and it would be unsafe to allow
    // the renderer to be destroyed on one thread while it could be used on another.
    // Kept in an optional so we can destruct it on "normal" exit, at the end of run_impl()
    // (can you guess from the length of this comment I am not satisfied by this design?)
    std::optional<T_renderer> mRenderer;

    std::queue<Operation> mOperations;
    std::mutex mOperationsMutex;
    imguiui::ImguiUi & mImguiUi;

    Controls mControls;

    std::atomic<bool> mThrew{false};
    std::exception_ptr mThreadException;

    std::atomic<bool> mStop{false};
    std::thread mThread;
};


} // namespace snac
} // namespace ad
