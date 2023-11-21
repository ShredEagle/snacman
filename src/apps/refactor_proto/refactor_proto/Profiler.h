#pragma once


#include <renderer/GL_Loader.h>

#include <array>
#include <memory>
#include <numeric>
#include <ostream>
#include <ratio>
#include <vector>


// Implementation notes:
// A profiler is one of those features that seem trivial, until you try to implement one.
// The design has several requirements:
// * The Profiler instance has knowledge of a "frame" or "step", inside which sections can be defined anywhere in client code.
// * The sections should not require client to provide permanent storage. 
//   * i.e., the Profiler is hosting all state that must be kept between frames.
//   * Exception: Sections guards on the stack are okay, because they are a common paradigm and only rely on local variables (not interframe).
// * Sections can be arbitrarily nested, and must always be balanced (client's responsibility).
//    * TODO Some metrics are harder to nest, for example OpenGL async Query objects,
//       where at most one/a few counter(s) for each target can be active at any time.
// * The number of sections is dynamic, and can grow (within machine limits).
// * TODO The (logical) identity of a section is not a trivial matter.
//   * All sections matching a common identity are grouped in a LogicalSection to match client's expectations.
//   * The identity should have implicit defaults (i.e. not client's responsiblity):
//     * If a section is inside a loop, each iteration should be cumulated 
//       * Only if the stack of parents is identical for each iteration, which should be true within a given loop execution
//         (but not necessaraly next time the same loop is reached).
//     * If a lexical section inside a scope is reached via distinct paths (path meaning the "stack of sections"), 
//       each path should lead to a distinct logical section.
//   * TODO What happens to identity when a frame composition changes (i.e. not the exact same sequence of sections)
//     * Number of iterations in a loop can differ very frequently (number of lights, culled objects)
//     * High level frame structure can differ (new screen effects, scene change (cut scene), ...)
//   * The client should be able to override default, with ability to override identity per section (e.g. ImGui Id-Stack).
// * Sections should be able to keep distinct metric of heterogenous types (CPU time, GPU time, GPU draw counts, ...)
//   * TODO The metrics should be user-extensible
//   * Restriction: a given logical section should list the same exact metrics at each frame.
//     * TODO What should profiler do if it does not?
// * TODO Some section are not recurring each frame, but are considered single shot events.
//   * They should not be discarded at the end of a frame, but only via explicit user request.
//   * They can exist outside of beginFrame()/endFrame().
//   * Should they be able to nest?
//   * What happen if several single-shot of the same name exist? Overwrite?

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


enum class EntryNature
{
    Recurring,
    SingleShot,
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
    virtual bool provide(EntryIndex aEntryIndex, std::uint32_t aQueryFrame, GLuint & aSampleResult) = 0;

    virtual void resize(std::size_t aNewEntriesCount) = 0;

    GLuint scale(GLuint aInput) const
    { return aInput * mScaleFactor.num / mScaleFactor.den; }

    const char * mQuantityName = "quantity";
    const char * mUnit = "u";
    Ratio mScaleFactor;
};


class Profiler
{
    /// @brief A logical section is what the clients expects to see as a single line in the profiler output,
    /// but could be consolidated from several distinct measures (e.g. loop consolidation).
    friend struct LogicalSection; // Implementation detail for grouping entries by logical section.

    /// @brief Delay before querying metrics.
    /// This is initially implemented so asynchronous GPU request have a good chance to be ready when queried.
    inline static constexpr std::uint32_t gFrameDelay{3};

public:
    inline static constexpr std::size_t gInitialEntries{256};
    inline static constexpr std::size_t gMaxSamples{128};
    inline static constexpr std::size_t gMaxMetricsPerSection{16};

    using EntryIndex = ProviderInterface::EntryIndex;
    using ProviderIndex = std::size_t;

    static std::uint32_t CountSubframes()
    { return gFrameDelay + 1; };

    Profiler();

    void beginFrame();
    void endFrame();

    EntryIndex beginSection(EntryNature aNature, const char * aName, std::initializer_list<ProviderIndex> aProviders);
    void endSection(EntryIndex aIndex);

    // I am not sure this is a good idea, as it relies on a Profiler global state (mCurrentParent), which might not be a good idea.
    // The current assumption is that sections should always be strictly nested, and that they are created on a single thread
    // (or at least that there is an independent copy of the Profiler state per thread).
    /// This helps with ending manual sections.
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
    
    SectionGuard scopeSection(EntryNature aNature, const char * aName, std::initializer_list<ProviderIndex> aProviders)
    { 
        return {
            .mProfiler = this,
            .mEntry = beginSection(aNature, aName, std::move(aProviders))
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
    /// @brief The current subframe, which should be provided to profilers beginSection() / endSection().
    std::uint32_t currentSubframe() const;

    /// @brief The subframe which is to be queried, taking into account the frame delay.
    std::uint32_t queriedSubframe() const;

    std::pair<EntryIndex, bool> setupNextEntryRecurring(const char * aName, auto aProviders);
    EntryIndex setupNextEntrySingleShot(const char * aName, auto aProviders);

    /// @brief A reference to a metrics Provider, and a `Value` record of the samples taken by this provider.
    template <class T_value, class T_average = T_value>
    struct Metric
    {
        //std::string mName;
        ProviderIndex mProviderIndex;
        Values<T_value, T_average> mValues;
    };

    // Forward
    struct FrameState;

    /// @brief A distinct Entry is used each time the flow of control reaches beginSection().
    /// (i.e. inside a given _frame_, each call to beginSection returns a distinct Entry.)
    /// @note Consolidation happens on the collection of Entries to group them by LogicalSection.
    ///
    /// Contains an array of `Metrics` measured for this entry.
    struct Entry
    {
        inline static constexpr unsigned int gInvalidLevel = -1;
        inline static constexpr unsigned int gSingleShotLevel = gInvalidLevel-1;
        inline static constexpr EntryIndex gInvalidEntry = std::numeric_limits<EntryIndex>::max();
        inline static constexpr EntryIndex gNoParent = gInvalidEntry - 1;

        struct Identity
        {
            const char * mName = nullptr;
            unsigned int mLevel = gInvalidLevel;
            EntryIndex mParentIdx = gInvalidEntry;

            bool operator==(const Identity & aRhs) const = default;
        };

        bool isUnused() const
        { return mId.mName == nullptr; }

        bool matchIdentity(const char * aName, const FrameState & aFrameState) const;

        /// @brief Returns true if `this` currently has exactly the same providers as the iterators range, in the same order.
        template <std::forward_iterator T_iterator>
        bool matchProviders(T_iterator aFirstProviderIdx, T_iterator aLastProviderIdx) const;

        template <std::forward_iterator T_iterator>
        void setProviders(T_iterator aFirstProviderIdx, T_iterator aLastProviderIdx);

        Identity mId;
        std::array<Metric<GLuint>, gMaxMetricsPerSection> mMetrics;
        std::size_t mActiveMetrics = 0;
    };

    /// @brief Returns the entry idx to use for the provided parameters, handling resizing of storage when needed.
    /// @warning Does advance the next entry, but does not set any value on the entry instance itself (not setting the name).
    EntryIndex fetchNextEntry(EntryNature aNature, const char * aName);

    void resize(std::size_t aNewEntriesCount);

    ProviderInterface & getProvider(const Metric<GLuint> & aMetric);
    const ProviderInterface & getProvider(const Metric<GLuint> & aMetric) const;

    /// @brief The intra-frame state (plus the corresponding frame number).
    struct FrameState
    {
        /// @brief Reset the frame state, except the frame number which is advanced.
        /// @note the current level and parent should already be at the initial value.
        void advanceFrame()
        {
            *this = FrameState{
                .mFrameNumber = mFrameNumber + 1,
            };
        };

        bool areAllSectionsClosed() const
        {
            return 
                (mCurrentLevel == 0)
                && (mCurrentParent == Entry::gNoParent)
            ;
        }

        EntryIndex mNextEntry{0};
        unsigned int mCurrentLevel{0};
        EntryIndex mCurrentParent{Entry::gNoParent};
        unsigned int mFrameNumber{std::numeric_limits<unsigned int>::max()};
    };

    std::vector<Entry> mEntries; // Initial size handled in constructor
    std::vector<EntryIndex> mSingleShots;
    std::vector<std::unique_ptr<ProviderInterface>> mMetricProviders;
    FrameState mFrameState;
    unsigned int mLastResetFrame{0};
};


} // namespace ad::renderer