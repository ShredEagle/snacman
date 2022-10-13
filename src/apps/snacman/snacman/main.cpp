#include "Logging.h"
#include "Timing.h"
#include "GraphicState.h"


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
    void run(StateFifo<GraphicState> & aStates)
    {
        SELOG(info)("Render thread started");

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

        while(true)
        {
            std::this_thread::sleep_for(ms{8});

            // TODO interpolate
            // TODO render
            SELOG(trace)("Render thread: Done (not) rendering.");

            if (auto latest = aStates.popToEnd(); latest)
            {
                entry = std::move(*latest);
                SELOG(trace)("Render thread: Newer state retrieved.");
            }
        }
    };

private:

};


int main(int argc, char * argv[])
{
    snac::detail::initializeLogging();
    spdlog::set_level(spdlog::level::trace);

    SELOG(info)("I'm a snac man.");
    SELOG(debug)("Delta time {}ms.", std::chrono::duration_cast<ms>(gSimulationDelta).count());

    StateFifo<GraphicState> graphicStates;
    std::thread RenderingThread{std::bind(&RenderThread::run, RenderThread{}, std::ref(graphicStates))};

    //
    // TODO Initialize scene
    //

    Clock::time_point endStepTime = Clock::now();

    while(true)
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
