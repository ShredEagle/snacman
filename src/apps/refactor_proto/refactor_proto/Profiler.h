#pragma once


#include <renderer/Query.h> 

#include <array>
#include <chrono>

// Implementation notes:
// A profiler is one of those feature that seems trivial, until you try to implement one.
// The design has several requirements:
// * The Profiler instance has knowledge of a "frame" or "step", inside which sections can be defined anywhere in client code.
// * The sections should not require client to provide permanent storage. 
//   * TODO Sections guards on the stack are okay.
//   * i.e., the Profiler is hosting all state that must be kept between frames.
// * Sections can be arbitrarily nested, and must always be balanced (client's responsibility).
//    * TODO Some metrics are harder to nest, for example OpenGL async Query objects,
//       where at most one/a few counter(s) for each target can be active at any time.
// * TODO The number of sections is dynamic, and can grow (within machine limits).
// * TODO The identity of a section is not a trivial matter
//   * The identity should have implicit defaults (i.e. not client's responsiblity):
//     * If a section is inside a loop, each iteration should be cumulated.
//     * If a lexical section inside a scope is reached via distinct paths, 
//       each path should lead to a distinct logical section.
//   * What happens to identity when a frame composition changes (i.e. not the exact same sequence of sections)
//     * Number of iterations in a loop can differ very frequently (number of lights, culled objects)
//     * High level frame structure can differ (new screen effects, scene change (cut scene), ...)
//   * The client should be able to override default, by opting-in per section.
// * TODO Sections should be able to keep distinct metric of heterogenous types (CPU time, GPU time, GPU draw counts, ...)
//   * The metrics should be user-extensible
//   * Restriction: a given logical section should list the same exact metrics at each frame.
//     * What should profiler do if it does not?
// * TODO Some section are not recurring each frame, but are considered single shot events.
//   * They should not be discarded at the end of a frame, but only via explicit user request.

namespace ad::renderer {


class ProviderInterface
{
public:
    using EntryIndex = std::size_t;

    ProviderInterface(const char * aQuantityName, const char * aUnit) :
        mQuantityName{aQuantityName},
        mUnit{aUnit}
    {}

    virtual ~ProviderInterface() = default;

    virtual void beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) = 0;
    virtual void endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) = 0;

    // TODO make generic regarding provided type
    virtual bool provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult) = 0;

    const char * mQuantityName = "quantity";
    const char * mUnit = "u";
};


class Profiler
{
public:
    inline static constexpr std::size_t gInitialEntries{64};
    inline static constexpr std::size_t gMaxSamples{128};
    inline static constexpr std::size_t gMaxMetricsPerSection{16};
    inline static constexpr std::uint32_t gFrameDelay{3};

    using EntryIndex = ProviderInterface::EntryIndex;
    using ProviderIndex = std::size_t;

    Profiler();

    void beginFrame();
    void endFrame();

    EntryIndex beginSection(const char * aName, std::initializer_list<ProviderIndex> aProviders);
    void endSection(EntryIndex aIndex);

    struct [[nodiscard]] SectionGuard
    {
        ~SectionGuard()
        {
            mProfiler->endSection(mEntry);
        }

        Profiler * mProfiler;
        EntryIndex mEntry;
    };
    
    SectionGuard scopeSection(const char * aName, std::initializer_list<ProviderIndex> aProviders)
    { 
        return {
            .mProfiler = this,
            .mEntry = beginSection(aName, std::move(aProviders))
        };
    }

    std::string prettyPrint() const;

private:
    template <class T_value, class T_average = T_value>
    struct Values
    {
        void record(T_value aSample)
        {
            mAccumulated += aSample - mSamples[mNextSampleIdx];
            mSamples[mNextSampleIdx] = aSample;
            mNextSampleIdx = (mNextSampleIdx + 1) % gMaxSamples;
            mSampleCount = std::min(mSampleCount + 1, gMaxSamples);
        }

        T_average average() const
        {
            if(mSampleCount == 0)
            {
                return 0;
            }
            else
            {
                return static_cast<T_average>(mAccumulated) / static_cast<T_average>(mSampleCount);
            }
        }

        T_value mAccumulated{0};
        std::array<T_value, gMaxSamples> mSamples{static_cast<T_value>(0)};
        std::size_t mNextSampleIdx{0};
        std::size_t mSampleCount{0};
    };

    template <class T_value, class T_average = T_value>
    struct Metric
    {
        //std::string mName;
        ProviderIndex mProviderIndex;
        Values<T_value, T_average> mValues;
    };

    struct Entry
    {
        inline static constexpr unsigned int gInvalidLevel = -1;

        const char * mName;
        unsigned int mLevel = gInvalidLevel;
        std::array<Metric<GLuint>, gMaxMetricsPerSection> mMetrics;
        std::size_t mActiveMetrics = 0;
    };

    ProviderInterface & getProvider(const Metric<GLuint> & aMetric);
    const ProviderInterface & getProvider(const Metric<GLuint> & aMetric) const;

    std::vector<Entry> mEntries{gInitialEntries};
    std::size_t mNextEntry{0};
    unsigned int mCurrentLevel{0};
    unsigned int mFrameNumber{std::numeric_limits<unsigned int>::max()};

    std::vector<std::unique_ptr<ProviderInterface>> mMetricProviders;
};


struct ProviderCPUTime : public ProviderInterface
{
    ProviderCPUTime() : 
        ProviderInterface{"CPU time", "us"}
    {}

    using Clock = std::chrono::high_resolution_clock;

    void beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;
    void endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;

    bool provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult) override;

    //////
    struct TimeInterval
    {
        Clock::time_point mBegin;
        Clock::time_point mEnd;
    };

    TimeInterval & getInterval(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame);

    std::array<TimeInterval, Profiler::gInitialEntries * Profiler::gFrameDelay> mTimePoints;
};


struct ProviderGL : public ProviderInterface
{
    ProviderGL();

    void beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;
    void endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;

    bool provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult) override;

    //////

    graphics::Query & getQuery(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame);

    std::vector<graphics::Query> mQueriesPool;
    inline static constexpr std::size_t gInitialQueriesInPool{Profiler::gInitialEntries * Profiler::gFrameDelay};

    bool mActive{false}; // There can be at most 1 query active per target (per index, for indexed targets)
};


struct ProviderGLTime : public ProviderInterface
{
    enum class Event
    {
        Begin,
        End,
    };

    ProviderGLTime() :
        ProviderInterface{"GPU time", "us"}
    {}

    void beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;
    void endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;

    bool provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult) override;

    //////
    graphics::Query & getQuery(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame, Event aEvent);

    // TODO Have a dynamic allocation size
    // This is currently very conservative, having enough queries to recorded begin/end time for each section
    // in each "frame delay slot".
    inline static constexpr std::size_t gInitialQueriesInPool{2 * Profiler::gInitialEntries * Profiler::gFrameDelay};

    std::vector<graphics::Query> mQueriesPool{gInitialQueriesInPool};
};

} // namespace ad::renderer