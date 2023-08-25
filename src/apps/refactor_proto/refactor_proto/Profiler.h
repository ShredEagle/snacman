#pragma once


#include "Time.h"

#include <renderer/Query.h> 

#include <array>
#include <numeric>


// Implementation notes:
// A profiler is one of those feature that seems trivial, until you try to implement one.
// The design has several requirements:
// * The Profiler instance has knowledge of a "frame" or "step", inside which sections can be defined anywhere in client code.
// * The sections should not require client to provide permanent storage. 
//   * Sections guards on the stack are okay.
//   * i.e., the Profiler is hosting all state that must be kept between frames.
// * Sections can be arbitrarily nested, and must always be balanced (client's responsibility).
//    * TODO Some metrics are harder to nest, for example OpenGL async Query objects,
//       where at most one/a few counter(s) for each target can be active at any time.
// * TODO The number of sections is dynamic, and can grow (within machine limits).
// * TODO The identity of a section is not a trivial matter
//   * The identity should have implicit defaults (i.e. not client's responsiblity):
//     * If a section is inside a loop, each iteration should be cumulated.
//     * If a lexical section inside a scope is reached via distinct paths (path meaning the "stack of sections"), 
//       each path should lead to a distinct logical section.
//   * What happens to identity when a frame composition changes (i.e. not the exact same sequence of sections)
//     * Number of iterations in a loop can differ very frequently (number of lights, culled objects)
//     * High level frame structure can differ (new screen effects, scene change (cut scene), ...)
//   * The client should be able to override default, with ability to overrid identity per section.
// * TODO Sections should be able to keep distinct metric of heterogenous types (CPU time, GPU time, GPU draw counts, ...)
//   * The metrics should be user-extensible
//   * Restriction: a given logical section should list the same exact metrics at each frame.
//     * What should profiler do if it does not?
// * TODO Some section are not recurring each frame, but are considered single shot events.
//   * They should not be discarded at the end of a frame, but only via explicit user request.

namespace ad::renderer {


/// @brief Runtime (dynamic) ratio, by opposition to the compile-time (static) std::ratio.
struct Ratio
{
    template<class T_ratio>
    static constexpr Ratio MakeRatio()
    {
        return Ratio{
            .num = T_ratio::num,
            .den = T_ratio::den,
        };
    }

    template<class T_ratioSource, class T_ratioDestination>
    static constexpr Ratio MakeConversion()
    {
        return MakeRatio<std::ratio_divide<T_ratioSource, T_ratioDestination>>();
    }

    static constexpr Ratio MakeConversion(Ratio aSource, Ratio aDestination)
    {
        Ratio r{
            .num = aSource.num * aDestination.den,
            .den = aSource.den * aDestination.num,
        };
        r.reduce();
        return r;
    }

    constexpr void reduce()
    {
        auto gcd = std::gcd(num, den);
        num /= gcd;
        den /= gcd;
    }

    GLuint num;
    GLuint den;
};


/// @brief Specialize this interface to provide new metrics.
class ProviderInterface
{
public:
    using EntryIndex = std::size_t;

    ProviderInterface(const char * aQuantityName, const char * aUnit, Ratio aScaleFactor = {.num = 1, .den = 1}) :
        mQuantityName{aQuantityName},
        mUnit{aUnit},
        mScaleFactor{aScaleFactor}
    {}

    virtual ~ProviderInterface() = default;

    virtual void beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) = 0;
    virtual void endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) = 0;

    // TODO make generic regarding provided type
    virtual bool provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult) = 0;

    virtual void resize(std::size_t aNewEntriesCount) = 0;

    GLuint scale(GLuint aInput) const
    { return aInput * mScaleFactor.num / mScaleFactor.den; }

    const char * mQuantityName = "quantity";
    const char * mUnit = "u";
    Ratio mScaleFactor;
};


class Profiler
{
    friend struct LogicalSection; // Implementation detail for grouping entries by logical section.

public:
    inline static constexpr std::size_t gInitialEntries{256};
    inline static constexpr std::size_t gMaxSamples{128};
    inline static constexpr std::size_t gMaxMetricsPerSection{16};
    /// @brief Delay before querying metrics.
    /// This is initially implemented so asynchronous GPU request have a good chance to be ready when queried.
    inline static constexpr std::uint32_t gFrameDelay{3};

    using EntryIndex = ProviderInterface::EntryIndex;
    using ProviderIndex = std::size_t;

    Profiler();

    void beginFrame();
    void endFrame();

    EntryIndex beginSection(const char * aName, std::initializer_list<ProviderIndex> aProviders);
    void endSection(EntryIndex aIndex);

    // I am not sure this is a good idea, but it relies on a Profiler global state (mCurrentParent) (which might not be a good idea)
    // The current assumption is that sections should always be strictly nested, and that they are created on a single thread
    // (or at least that there is an independent copy of the Profiler state per thread).
    // This helps with ending manual sections.
    void popCurrentSection();

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

    void prettyPrint(std::ostream & aOut) const;

    // Note: struct Values is kept public because it is a convenient utility class.
    /// @brief Keep a record of up to `gMaxSample` samples, as well as special values over this record (TODO e.g. min, max, accumulation).
    /// @tparam T_value The type of value that are recorded when sampling.
    /// @tparam T_average The type of the average, notably usefull to request decimals when averaging integers.
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

private:
    /// @brief A reference to a metrics Provider, and a `Value` record of the samples taken by this provider.
    template <class T_value, class T_average = T_value>
    struct Metric
    {
        //std::string mName;
        ProviderIndex mProviderIndex;
        Values<T_value, T_average> mValues;
    };

    /// @brief A distinct Entry is used each time the flow of control reaches beginSection().
    /// (i.e. inside a given _frame_, each call to beginSection returns a distinct Entry.)
    ///
    /// Contains an array of `Metrics` measured for this entry.
    struct Entry
    {
        inline static constexpr unsigned int gInvalidLevel = -1;
        inline static constexpr EntryIndex gInvalidEntry = std::numeric_limits<EntryIndex>::max();
        inline static constexpr EntryIndex gNoParent = gInvalidEntry - 1;

        struct Identity
        {
            const char * mName;
            unsigned int mLevel = gInvalidLevel;
            EntryIndex mParentIdx = gInvalidEntry;

            bool operator==(const Identity & aRhs) const = default;
        };

        Identity mId;
        std::array<Metric<GLuint>, gMaxMetricsPerSection> mMetrics;
        std::size_t mActiveMetrics = 0;
    };

    /// @brief Returns the next entry, handling resizing of storage when needed.
    Entry & getNextEntry();

    void resize(std::size_t aNewEntriesCount);

    ProviderInterface & getProvider(const Metric<GLuint> & aMetric);
    const ProviderInterface & getProvider(const Metric<GLuint> & aMetric) const;

    std::vector<Entry> mEntries; // Initial size handled in constructor
    EntryIndex mNextEntry{0};
    unsigned int mCurrentLevel{0};
    EntryIndex mCurrentParent{Entry::gNoParent};
    unsigned int mFrameNumber{std::numeric_limits<unsigned int>::max()};

    std::vector<std::unique_ptr<ProviderInterface>> mMetricProviders;
};


struct ProviderCPUTime : public ProviderInterface {
    ProviderCPUTime() : 
        ProviderInterface{"CPU time",
                          "us",
                           Ratio::MakeConversion<Clock::period, std::micro>()}
    {}

    void beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;
    void endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;

    bool provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult) override;

    void resize(std::size_t aNewEntriesCount) override
    { mTimePoints.resize(aNewEntriesCount * Profiler::gFrameDelay); }

    //////
    struct TimeInterval
    {
        Clock::time_point mBegin;
        Clock::time_point mEnd;
    };

    TimeInterval & getInterval(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame);

    std::vector<TimeInterval> mTimePoints;
};


struct ProviderGL : public ProviderInterface
{
    ProviderGL() :
        ProviderInterface{"GPU generated", "primitive(s)"}
    {}

    void beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;
    void endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;

    bool provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult) override;

    void resize(std::size_t aNewEntriesCount) override
    { mQueriesPool.resize(aNewEntriesCount * Profiler::gFrameDelay); }

    //////

    graphics::Query & getQuery(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame);

    std::vector<graphics::Query> mQueriesPool;
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

    // This is currently very conservative, having enough queries to recorded begin/end time for each section
    // in each "frame delay slot".
    void resize(std::size_t aNewEntriesCount) override
    { mQueriesPool.resize(2 * aNewEntriesCount * Profiler::gFrameDelay); }

    //////
    graphics::Query & getQuery(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame, Event aEvent);

    std::vector<graphics::Query> mQueriesPool;
};

} // namespace ad::renderer