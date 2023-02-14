#pragma once


#include "GraphicState.h"
#include "Logging.h"
#include "Profiling.h"
#include "Timing.h"

#include <graphics/ApplicationGlfw.h>

#include <imguiui/ImguiUi.h>

#include <resource/ResourceFinder.h>

#include <snac-renderer/Camera.h>
#include <snac-renderer/Mesh.h>
#include <snac-renderer/text/Text.h>

#include <future>
#include <queue>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX 
#include <windows.h>
#include <processthreadsapi.h>
#undef NOMINMAX 
#undef WIN32_LEAN_AND_MEAN 
#endif

namespace ad {
namespace snac {


class Resources;


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

public:
    RenderThread(graphics::ApplicationGlfw & aGlfwApp,
                 GraphicStateFifo_t & aStates,
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

    std::future<std::shared_ptr<snac::Mesh>> loadShape(Resources & aResources)
    {
        // std::function require the type-erased functor to be copy constructible.
        // all captured types must be copyable.
        auto promise = std::make_shared<std::promise<std::shared_ptr<snac::Mesh>>>();
        std::future<std::shared_ptr<snac::Mesh>> future = promise->get_future();
        push([promise = std::move(promise), &aResources]
             (T_renderer & aRenderer) 
             {
                promise->set_value(aRenderer.LoadShape(aResources));
             });
        return future;
    }

    std::future<std::shared_ptr<snac::Font>> loadFont(
        filesystem::path aFont,
        unsigned int aPixelHeight,
        Resources & aResources)
    {
        // std::function require the type-erased functor to be copy constructible.
        // all captured types must be copyable.
        auto promise = std::make_shared<std::promise<std::shared_ptr<snac::Font>>>();
        std::future<std::shared_ptr<snac::Font>> future = promise->get_future();
        push([promise = std::move(promise), font = std::move(aFont), aPixelHeight, &aResources]
             (T_renderer & aRenderer) mutable
             {
                promise->set_value(aRenderer.loadFont(font, aPixelHeight, aResources));
             });
        return future;
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

    void run(GraphicStateFifo_t & aStates, T_renderer && aRenderer)
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

    void run_impl(GraphicStateFifo_t & aStates, T_renderer && aRenderer)
    {
        SELOG(info)("Render thread started");

#if defined(_WIN32)
        HRESULT r;
        r = SetThreadDescription(GetCurrentThread(), L"Render Thread");
#endif

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

        // If the client is waiting on a future from this thread 
        // before producing the two first states, we must service to avoid a deadlock.
        while(aStates.size() < EntryBuffer<T_renderer>::BufferDepth)
        {
            // Note: this is busy looping at the moment.
            // This should only be for a brief period at the beginning.
            serviceOperations(aRenderer);
        }

        // Initialize the buffer with the required number of entries (2 for
        // double buffering) This is blocking call, done last to offer more
        // opportunities for the entries to already be in the queue
        EntryBuffer<T_renderer> entries{aStates};
        SELOG(info)("Render thread initialized states buffer.");

        while (!mStop)
        {
            Guard frameProfiling = profileFrame(snac::Profiler::Render);

            // Service all queued operations first.
            serviceOperations(aRenderer);

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
                TIME_RECURRING(Render, "Interpolation");

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

            {
                TIME_RECURRING(Render, "ImGui::renderBackend");
                mImguiUi.renderBackend();
            }

            {
                TIME_RECURRING(Render, "Swap buffers");
                mApplication.swapBuffers();
            }
            getRenderProfilerPrint().print();
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


} // namespace snac
} // namespace ad