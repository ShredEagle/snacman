#include "profiler.h"

#include "Logging.h"

#include <cassert>


namespace ad::renderer {


Profiler::Profiler()
{
    mMetricProviders.push_back(std::make_unique<ProviderCPUTime>());
    mMetricProviders.push_back(std::make_unique<ProviderGLTime>());
    mMetricProviders.push_back(std::make_unique<ProviderGL>());
}


ProviderInterface & Profiler::getProvider(const Metric<GLuint> & aMetric)
{
    return *mMetricProviders.at(aMetric.mProviderIndex);
}


const ProviderInterface & Profiler::getProvider(const Metric<GLuint> & aMetric) const
{
    return *mMetricProviders.at(aMetric.mProviderIndex);
}


void Profiler::beginFrame()
{
    ++mFrameNumber;
    mNextEntry = 0;
    mCurrentLevel = 0; // should already be the case
    mCurrentParent = Entry::gNoParent; // should already be the case
}

void Profiler::endFrame()
{
    assert(mCurrentLevel == 0); // sanity check
    assert(mCurrentParent == Entry::gNoParent); // sanity check

    // Ensure we have enough entries in the circular buffer (frame count >= gFrameDelay)
    if((mFrameNumber + 1) < gFrameDelay)
    {
        return;
    }

    for(EntryIndex i = 0; i != mNextEntry; ++i)
    {
        Entry & entry = mEntries[i];
        std::uint32_t queryFrame = (mFrameNumber + 1) % gFrameDelay;

        GLuint result;
        for (std::size_t metricIdx = 0; metricIdx != entry.mActiveMetrics; ++metricIdx)
        {
            if(getProvider(entry.mMetrics[metricIdx]).provide(i, queryFrame, result))
            {
                entry.mMetrics[metricIdx].mValues.record(result);
            }
            else
            {
                SELOG(error)("A provider result was not available when queried, it is discarded.");
            }
        }
    }
}

Profiler::EntryIndex Profiler::beginSection(const char * aName, std::initializer_list<ProviderIndex> aProviders)
{
    // TODO Section identity is much more subtle than that
    // It should be robust to changing the structure of sections (loop variance, and logical structure)
    // and handle change in providers somehow.
    // For the moment, make a few assertions that will trip the program as soon as it becomes more dynamic.

    // There must be a number of provider in [1, maxMetricsPerSection]
    assert(aProviders.size() > 0 && aProviders.size() <= gMaxMetricsPerSection);
    
    Entry & entry = mEntries[mNextEntry];

    // Because we do not track identity properly when frame structure change, we hope it always matches
    assert(entry.mId.mName == nullptr || entry.mId.mName == aName);
    assert(entry.mId.mLevel == Entry::gInvalidLevel || entry.mId.mLevel == mCurrentLevel);
    assert(entry.mId.mParentIdx == Entry::gInvalidEntry || entry.mId.mParentIdx == mCurrentParent);

    entry.mId.mName = aName;
    entry.mId.mLevel = mCurrentLevel++;
    entry.mId.mParentIdx = mCurrentParent;
    mCurrentParent = mNextEntry; //i.e. this Entry index becomes the current parent

    if(entry.mActiveMetrics == 0)
    {
        for(const ProviderIndex providerIndex : aProviders)
        {
            entry.mMetrics.at(entry.mActiveMetrics++).mProviderIndex = providerIndex;
        }
    }

    // Also part of identity, a logical section should always be given the exact same list of providers.
    assert(entry.mActiveMetrics == aProviders.size());

    for (std::size_t i = 0; i != entry.mActiveMetrics; ++i)
    {
        getProvider(entry.mMetrics[i]).beginSection(mNextEntry, mFrameNumber % gFrameDelay);
    }

    return mNextEntry++;
}


void Profiler::endSection(EntryIndex aIndex)
{
    Entry & entry = mEntries.at(aIndex);
    for (std::size_t i = 0; i != entry.mActiveMetrics; ++i)
    {
        getProvider(entry.mMetrics[i]).endSection(aIndex, mFrameNumber % gFrameDelay);
    }
    mCurrentParent = entry.mId.mParentIdx; // restore this Entry parent as the current parent
    --mCurrentLevel;
}


struct LogicalSection
{
    LogicalSection(const Profiler::Entry & aEntry, Profiler::EntryIndex aEntryIndex) :
        mId{aEntry.mId},
        mValues{aEntry.mMetrics},
        mActiveMetrics{aEntry.mActiveMetrics},
        mEntries{aEntryIndex}
    {}

    void push(const Profiler::Entry & aEntry, Profiler::EntryIndex aEntryIndex)
    {
        assert(mActiveMetrics == aEntry.mActiveMetrics);
        for (std::size_t metricIdx = 0; metricIdx != mActiveMetrics; ++metricIdx)
        {
            // TODO the accumulation should also handle other data (samples, max, min, ...)
            // could be encapsulated in a Value member function.
            assert(mValues[metricIdx].mProviderIndex == aEntry.mMetrics[metricIdx].mProviderIndex);
            mValues[metricIdx].mValues.mAccumulated += aEntry.mMetrics[metricIdx].mValues.mAccumulated;
        }
        mEntries.push_back(aEntryIndex);
    }

    /// @brief Identity that determines the belonging of an Entry to this logical Section.
    Profiler::Entry::Identity mId;
    // TODO should we use an array? 
    // Is it really useful to keep if we cumulate at push? (might be if we cache the sort result between frames)
    std::vector<Profiler::EntryIndex> mEntries;

    // TODO we actually only need to store Values here (not even sure we should keep the individual samples)
    // If we go this route, we should split Metrics from AOS to SOA so we can copy Values from Entries with memcpy
    std::array<Profiler::Metric<GLuint>, Profiler::gMaxMetricsPerSection> mValues;
    std::size_t mActiveMetrics = 0;
};


std::string Profiler::prettyPrint() const
{
    using Clock = ProviderCPUTime::Clock;
    auto beginTime = Clock::now();

    //
    // Group by identity
    //

    // TODO Ad 2023/08/03: #sectionid Since for the moment we expect the "frame entry structure" to stay identical
    // we actually do not need to sort again at each frame.
    // In the long run though, we probably want to handle frame structure changes,
    // but there might be an optimization when we can guarantee the structure is identical to previous frame.
    std::vector<LogicalSection> sections; // Note: We know there are at most mNextEntry logical sections,
                                          // might allow for a preallocated array

    using LogicalSectionId = unsigned short; // we should not have that many logical sections right?
    static constexpr LogicalSectionId gInvalidLogicalSection = std::numeric_limits<LogicalSectionId>::max();
    std::vector<LogicalSectionId> entryIdToLogicalSectionId(mEntries.size(), gInvalidLogicalSection);

    for(EntryIndex entryIdx = 0; entryIdx != mNextEntry; ++entryIdx)
    {
        const Entry & entry = mEntries[entryIdx];

        // Important:
        // We cannot simply compare the Entry Identity the Identity of a section to test if the Entry belongs there:
        // the parent index could be different (i.e. parents are distinct entries),
        // yet both parents could still belong to the same section.
        // For this reason, we actually have to lookup which section each parent belongs to,
        // this is the purpose of the lookup below.
        if(auto found = std::find_if(sections.begin(), sections.end(),
                                     [&entry, &lookup = entryIdToLogicalSectionId](const auto & aCandidateSection)
                                     {
                                        assert(entry.mId.mParentIdx == Entry::gNoParent 
                                            || (lookup[entry.mId.mParentIdx] != gInvalidLogicalSection));

                                        return aCandidateSection.mId.mName == entry.mId.mName
                                                // Check if the entry and the candidate section have the same logical parent:
                                                // * either both have a parent, then we compare the LogicalSectionId of both parents
                                                // * OR both have no parent
                                            && (   (   entry.mId.mParentIdx != Entry::gNoParent 
                                                    && aCandidateSection.mId.mParentIdx != Entry::gNoParent 
                                                    && lookup[entry.mId.mParentIdx] == lookup[aCandidateSection.mId.mParentIdx])
                                                || (   entry.mId.mParentIdx == Entry::gNoParent
                                                    && aCandidateSection.mId.mParentIdx == Entry::gNoParent))
                                                // Check if the entry and candidate have the same level
                                                // Should alway be true then, so we assert
                                            && (assert(aCandidateSection.mId.mLevel == entry.mId.mLevel), true)
                                            ;
                                     });
           found != sections.end())
        {
            entryIdToLogicalSectionId[entryIdx] = (LogicalSectionId)(found - sections.begin());
            found->push(entry, entryIdx);
        }
        else
        {
            entryIdToLogicalSectionId[entryIdx] = (LogicalSectionId)sections.size();
            sections.emplace_back(entry, entryIdx);
        }
    }

    auto sortedTime = Clock::now();

    //
    // Write the aggregated results to output stream
    //
    std::ostringstream oss;
    for(const LogicalSection & section : sections)
    {
        oss << std::string(section.mId.mLevel * 2, ' ') << section.mId.mName << ":" 
            ;
        
        for (std::size_t valueIdx = 0; valueIdx != section.mActiveMetrics; ++valueIdx)
        {
            const auto & metric = section.mValues[valueIdx];
            const ProviderInterface & provider = getProvider(metric);

            oss << " " << provider.mQuantityName << " " 
                << metric.mValues.average() 
                << " " << provider.mUnit << ","
                ;
        }

        if(section.mEntries.size() > 1)
        {
            oss << " (accumulated: " << section.mEntries.size() << ")";
        }

        oss << "\n";
    }

    //
    // Output timing statistics about this function itself
    //
    {
        using std::chrono::microseconds;
        oss << "(generated from "
            << mNextEntry << " entries in " 
            << ProviderCPUTime::GetTicks<microseconds>(Clock::now() - beginTime) << " us"
            << ", sort took " << ProviderCPUTime::GetTicks<microseconds>(sortedTime - beginTime) << " us"
            << ")"
            ;
    }

    return oss.str();
}


///////////////
// Providers //
///////////////

ProviderCPUTime::TimeInterval & ProviderCPUTime::getInterval(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    return mTimePoints[aEntryIndex * Profiler::gFrameDelay + aCurrentFrame];
}


void ProviderCPUTime::beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    getInterval(aEntryIndex, aCurrentFrame).mBegin = Clock::now();
}


void ProviderCPUTime::endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    getInterval(aEntryIndex, aCurrentFrame).mEnd = Clock::now();
}


bool ProviderCPUTime::provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult)
{
    const auto & interval = getInterval(aEntryIndex, aQueryFrame);
    aSampleResult = GetTicks<std::chrono::microseconds>(interval.mEnd - interval.mBegin);
    return true;
}


ProviderGL::ProviderGL() :
    ProviderInterface{"GPU generated", "primitive(s)"},
    mQueriesPool{gInitialQueriesInPool}
{}


graphics::Query & ProviderGL::getQuery(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    return mQueriesPool[aEntryIndex * Profiler::gFrameDelay + aCurrentFrame];
}


void ProviderGL::beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    assert(!mActive);
    mActive = true;
    glBeginQueryIndexed(GL_PRIMITIVES_GENERATED, 0, getQuery(aEntryIndex, aCurrentFrame));
}


void ProviderGL::endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    glEndQueryIndexed(GL_PRIMITIVES_GENERATED, 0);
    mActive = false;
}


template <class T_result>
bool getGLQueryResult(const graphics::Query & aQuery, T_result & aResult)
{
    GLuint available;
    glGetQueryObjectuiv(aQuery, GL_QUERY_RESULT_AVAILABLE, &available);

    if(available)
    {
        if constexpr(std::is_same_v<T_result, GLuint>)
        {
            glGetQueryObjectuiv(aQuery, GL_QUERY_RESULT, &aResult);
        }
        else if constexpr(std::is_same_v<T_result, GLuint64>)
        {
            glGetQueryObjectui64v(aQuery, GL_QUERY_RESULT, &aResult);
        }
    }

    return available != GL_FALSE;
}


bool ProviderGL::provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult)
{
    return getGLQueryResult(getQuery(aEntryIndex, aQueryFrame), aSampleResult);
}


graphics::Query & ProviderGLTime::getQuery(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame, Event aEvent)
{
    return mQueriesPool[2 * (aEntryIndex * Profiler::gFrameDelay + aCurrentFrame) 
                        + (aEvent == Event::Begin ? 0 : 1)];
}

void ProviderGLTime::beginSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    glQueryCounter(getQuery(aEntryIndex, aCurrentFrame, Event::Begin), GL_TIMESTAMP);
}


void ProviderGLTime::endSection(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    glQueryCounter(getQuery(aEntryIndex, aCurrentFrame, Event::End), GL_TIMESTAMP);
}


bool ProviderGLTime::provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult)
{
    GLuint64 beginTime, endTime; 
    bool available = getGLQueryResult(getQuery(aEntryIndex, aQueryFrame, Event::Begin), beginTime);
    available &= getGLQueryResult(getQuery(aEntryIndex, aQueryFrame, Event::End), endTime);

    aSampleResult = static_cast<GLuint>((endTime - beginTime) / 1000);
    return available;
}


} // namespace ad::renderer