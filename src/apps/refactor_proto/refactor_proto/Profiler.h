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

namespace ad::renderer {


class ProviderInterface
{
public:
    using EntryIndex = std::size_t;

    virtual void beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) = 0;
    virtual void endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) = 0;

    // TODO make generic regarding provided type
    virtual bool provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult) = 0;
};


class Profiler
{
public:
    inline static constexpr std::size_t gInitialEntries{64};
    inline static constexpr std::size_t gMaxSamples{128};
    inline static constexpr std::uint32_t gFrameDelay{3};

    using EntryIndex = ProviderInterface::EntryIndex;

    Profiler();

    void beginFrame();
    void endFrame();

    EntryIndex beginSection(const char * aName);
    void endSection(EntryIndex aIndex);

    //struct [[nodiscard]] Section
    //{
    //    
    //};
    //
    //Section measure(GLenum aQueryTarget);

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
        using ProviderFunction = std::function<bool(uint32_t /*aQueryFrame*/, T_value & /*aSampleResult*/)>;

        //std::string mName;
        Values<T_value, T_average> mValues;
        //std::array<ProviderFunction, gFrameDelay> providersBuffer{nullptr};
    };

    struct Entry
    {
        const char * mName;
        unsigned int mLevel = 0;
        Metric<GLuint> mCpuDurations;
        Metric<GLuint> mPrimitivesGenerated;
    };

    std::vector<Entry> mEntries{gInitialEntries};
    std::size_t mNextEntry{0};
    unsigned int mCurrentLevel{0};
    unsigned int mFrameNumber{std::numeric_limits<unsigned int>::max()};

    std::unique_ptr<ProviderInterface> mDurationsProvider;
    std::unique_ptr<ProviderInterface> mPrimitivesProvider;
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


struct ProviderCPUTime : public ProviderInterface
{
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





} // namespace ad::renderer