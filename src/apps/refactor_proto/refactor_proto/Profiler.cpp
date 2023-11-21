#include "Profiler.h"

#include "Logging.h"


#include "providers/ProviderApi.h"
#include "providers/ProviderCpu.h"
#include "providers/ProviderGL.h"


#if defined(_WIN32)
#include "providers/ProviderRdtsc.h"
#include "providers/ProviderWindows.h"
#endif

#include <cassert>


namespace ad::renderer {


template <class T_value>
bool Profiler::Entry<T_value>::matchIdentity(const char * aName, const FrameState & aFrameState) const
{
    return 
        (mId.mName == aName)
        && (mId.mLevel == aFrameState.mCurrentLevel)
        && (mId.mParentIdx == aFrameState.mCurrentParent)
    ;
}


template <class T_value>
template <std::forward_iterator T_iterator>
bool Profiler::Entry<T_value>::matchProviders(T_iterator aFirstProviderIdx, T_iterator aLastProviderIdx) const
{
    return std::equal(mMetrics.begin(), mMetrics.begin() + mActiveMetrics,
                      aFirstProviderIdx, aLastProviderIdx,
                      [](const Profiler::Metric<T_value> & aMetric, Profiler::ProviderIndex aProviderIdx)
                      {
                          return aMetric.mProviderIndex == aProviderIdx;
                      });
}


Profiler::Profiler()
{
#if defined(_WIN32)
    mMetricProviders.push_back(std::make_unique<ProviderCpuRdtsc>());
    //mMetricProviders.push_back(std::make_unique<ProviderCpuPerformanceCounter>());
#else
    mMetricProviders.push_back(std::make_unique<ProviderCPUTime>());
#endif

    mMetricProviders.push_back(std::make_unique<ProviderGLTime>());
    mMetricProviders.push_back(std::make_unique<ProviderGL>());
    mMetricProviders.push_back(std::make_unique<ProviderApi<&GlApi::Metrics::drawCount>>("draw", ""));
    mMetricProviders.push_back(std::make_unique<ProviderApi<&GlApi::Metrics::bufferMemoryWritten>>("buffer w", "B"));

    resize(gInitialEntries);
}


Profiler::RecurringEntry & Profiler::fetchNextEntry()
{
    if(mFrameState.mNextEntry == mRecurringEntries.size())
    {
        auto newSize = mRecurringEntries.size() * 2;
        SELOG(debug)("Resizing the profiler to {} entries.", newSize);
        resize(newSize);
    }
    return mRecurringEntries[mFrameState.mNextEntry];
}


void Profiler::resize(std::size_t aNewEntriesCount)
{
    mRecurringEntries.resize(aNewEntriesCount);
    for(const auto & provider : mMetricProviders)
    {
        provider->resize(aNewEntriesCount);
    }
}


template <class T_value>
ProviderInterface & Profiler::getProvider(const Metric<T_value> & aMetric)
{
    return *mMetricProviders.at(aMetric.mProviderIndex);
}


template <class T_value>
const ProviderInterface & Profiler::getProvider(const Metric<T_value> & aMetric) const
{
    return *mMetricProviders.at(aMetric.mProviderIndex);
}


std::uint32_t Profiler::currentSubframe() const
{
    return mFrameState.mFrameNumber % CountSubframes();
}


std::uint32_t Profiler::queriedSubframe() const
{
    return (mFrameState.mFrameNumber + 1) % CountSubframes();
}


void Profiler::beginFrame()
{
    mFrameState.advanceFrame();
}


void Profiler::endFrame()
{
    assert(mFrameState.areAllSectionsClosed());

    // TODO Ad 2023/11/15: I do not like this explicit reset of all values at the end of the frame if a reset occurred:
    // this is wasteful of resources, since all values for entities that changed have already been reset
    // But this is currently imposed by the prettyPrint, which expects all Entities of a logical section to have the same count of samples...
    if(mFrameState.mFrameNumber == mLastResetFrame)
    {
        // If a reset occured during this frame, reset all values in all entries
        for(EntryIndex entryIdx = 0; entryIdx != mFrameState.mNextEntry; ++entryIdx)
        {
            RecurringEntry & entry = mRecurringEntries[entryIdx];
            for (std::size_t metricIdx = 0; metricIdx != entry.mActiveMetrics; ++metricIdx)
            {
                entry.mMetrics[metricIdx].mValues = {};
            }
        }
    }

    // Ensure we have enough entries in the circular buffer before issuing queries (frame count since reset >= gFrameDelay)
    if(mFrameState.mFrameNumber < (mLastResetFrame + gFrameDelay))
    {
        return;
    }

    for(EntryIndex entryIdx = 0; entryIdx != mFrameState.mNextEntry; ++entryIdx)
    {
        RecurringEntry & entry = mRecurringEntries[entryIdx];
        std::uint32_t queryFrame = queriedSubframe();

        GLuint result;
        for (std::size_t metricIdx = 0; metricIdx != entry.mActiveMetrics; ++metricIdx)
        {
            if(getProvider(entry.mMetrics[metricIdx]).provide(entryIdx, queryFrame, result))
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

Profiler::EntryIndex Profiler::beginSectionRecurring(const char * aName, std::initializer_list<ProviderIndex> aProviders)
{
    // TODO Section identity is much more subtle than that
    // It should be robust to changing the structure of sections (loop variance, and logical structure)
    // and handle change in providers somehow.
    // For the moment, make a few assertions that will trip the program as soon as structure changes.

    // There must be a number of provider in [1, maxMetricsPerSection]
    assert(aProviders.size() > 0 && aProviders.size() <= gMaxMetricsPerSection);
    
    auto setupNextEntry = [this](const char * aName, auto aProviders) -> std::pair<EntryIndex, bool>
    {
        RecurringEntry & entry = fetchNextEntry();
        bool alreadyPresent;

        if(alreadyPresent = entry.matchIdentity(aName, mFrameState);
           alreadyPresent)
        {
            // Also part of identity, a logical section should always be given the exact same list of providers.
            // For the moment, we consider it a logic error if the identity changes only because of a different provider list
            // (this assumption can be revised)
            assert(entry.matchProviders(aProviders.begin(), aProviders.end()));
        }
        else // Either a newer used entry, or the frame structure changed and the entry was used for another identity
        {
            entry.mId.mName = aName;
            entry.mId.mLevel = mFrameState.mCurrentLevel;
            entry.mId.mParentIdx = mFrameState.mCurrentParent;

            entry.mActiveMetrics = 0;
            [[maybe_unused]] ProviderIndex previousProvider = std::numeric_limits<ProviderIndex>::max(); // only used for the sorting assertion
            for(const ProviderIndex providerIndex : aProviders)
            {
                // Provider indices must always be sorted (this is notably an assumption in LogicalSection::accountForChildSection)
                assert(previousProvider == std::numeric_limits<ProviderIndex>::max() || previousProvider < providerIndex);
                previousProvider = providerIndex;

                entry.mMetrics.at(entry.mActiveMetrics++) = RecurringEntry::Metric_t{
                    // TODO we could optimize that without rewritting each individual sample to zero
                    .mProviderIndex = providerIndex,
                    .mValues = {}, // Note: No effect line, to make explicit that we reset values.
                };
            }
        }

        ++mFrameState.mCurrentLevel;
        mFrameState.mCurrentParent = mFrameState.mNextEntry; //i.e. this Entry index becomes the current parent
        return {mFrameState.mNextEntry++, alreadyPresent};
    };

    auto [entryIndex, alreadyPresent] = setupNextEntry(aName, aProviders);
    RecurringEntry & entry = mRecurringEntries[entryIndex];

    if (!alreadyPresent)
    {
        mLastResetFrame = mFrameState.mFrameNumber;
        SELOG(debug)("Profiler sections structure changed.");
    }

    for (std::size_t i = 0; i != entry.mActiveMetrics; ++i)
    {
        getProvider(entry.mMetrics[i]).beginSectionRecurring(entryIndex, currentSubframe());
    }

    return entryIndex;
}


void Profiler::endSectionRecurring(EntryIndex aIndex)
{
    RecurringEntry & entry = mRecurringEntries.at(aIndex);
    for (std::size_t i = 0; i != entry.mActiveMetrics; ++i)
    {
        getProvider(entry.mMetrics[i]).endSectionRecurring(aIndex, currentSubframe());
    }
    mFrameState.mCurrentParent = entry.mId.mParentIdx; // restore this Entry parent as the current parent
    --mFrameState.mCurrentLevel;
}


void Profiler::popCurrentSection()
{
    endSectionRecurring(mFrameState.mCurrentParent);
}


struct LogicalSection
{
    LogicalSection(const Profiler::RecurringEntry & aEntry, Profiler::EntryIndex aEntryIndex) :
        mId{aEntry.mId},
        mEntries{aEntryIndex},
        mValues{aEntry.mMetrics},
        mActiveMetrics{aEntry.mActiveMetrics}
    {}

    void push(const Profiler::RecurringEntry & aEntry, Profiler::EntryIndex aEntryIndex)
    {
        assert(mActiveMetrics == aEntry.mActiveMetrics);
        for (std::size_t metricIdx = 0; metricIdx != mActiveMetrics; ++metricIdx)
        {
            // TODO the accumulation should also handle other data (current sample, max, min, ...)
            // could be encapsulated in a Value member function.
            assert(mValues[metricIdx].mProviderIndex == aEntry.mMetrics[metricIdx].mProviderIndex);
            // Average will use the sample count (copied from the Entry provided on this construction)
            assert(mValues[metricIdx].mValues.mSampleCount == aEntry.mMetrics[metricIdx].mValues.mSampleCount);
            mValues[metricIdx].mValues.mAccumulated += aEntry.mMetrics[metricIdx].mValues.mAccumulated;
        }
        mEntries.push_back(aEntryIndex);
    }

    void accountChildSection(const LogicalSection & aNested)
    {
        std::size_t thisMetricIdx = 0, childMetricIdx = 0;
        while(thisMetricIdx != mActiveMetrics && childMetricIdx != aNested.mActiveMetrics)
        {
            if(mValues[thisMetricIdx].mProviderIndex == aNested.mValues[childMetricIdx].mProviderIndex)
            {
                mAccountedFor[thisMetricIdx] += aNested.mValues[childMetricIdx].mValues.average();
                ++thisMetricIdx;
                ++childMetricIdx;
            }
            else if(mValues[thisMetricIdx].mProviderIndex < aNested.mValues[childMetricIdx].mProviderIndex)
            {
                ++thisMetricIdx;
            }
            else
            {
                ++childMetricIdx;
            }
        }
    }

    /// @brief Identity that determines the belonging of an Entry to this logical Section.
    Profiler::RecurringEntry::Identity mId;
    // TODO should we use an array? 
    // Is it really useful to keep if we cumulate at push? (might be if we cache the sort result between frames)
    /// @brief Collection of entries beloging to this logical section. 
    std::vector<Profiler::EntryIndex> mEntries;

    // TODO we actually only need to store Values here (not even sure we should keep the individual samples)
    // (note: we do not accumulate the samples at the moment)
    // If we go this route, we should split Metrics from AOS to SOA so we can copy Values from Entries with memcpy
    std::array<Profiler::RecurringEntry::Metric_t, Profiler::gMaxMetricsPerSection> mValues;
    std::size_t mActiveMetrics = 0;

    /// @brief Keep track of the samples accounted for by sub-sections 
    /// (thus allowing to know what has not been accounted for)
    std::array<GLuint, Profiler::gMaxMetricsPerSection> mAccountedFor{0}; 
};


// Note: It is currently required to happen outside a profiler frame, so endFrame() had reset all entries Values if it was a reset frame.
// (otherwise, it triggers an assert because Entries of the same LogicalSection might have different number of samples)
void Profiler::prettyPrint(std::ostream & aOut) const
{
    // Not a hard requirement currently, but it seems more sane.
    // (actually, a better test would be checking this is not called between beginFrame() and endFrame())
    assert(mFrameState.areAllSectionsClosed());

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

    using LogicalSectionId = unsigned short; // we should not have that many logical sections as to require a ushort, right?
    static constexpr LogicalSectionId gInvalidLogicalSection = std::numeric_limits<LogicalSectionId>::max();
    // This vector associate each entry index to a logical section index.
    // It allows to test whether distinct entry indices correspond to the same logical section, for parent consolidation.
    std::vector<LogicalSectionId> entryIdToLogicalSectionId(mRecurringEntries.size(), gInvalidLogicalSection);

    for(EntryIndex entryIdx = 0; entryIdx != mFrameState.mNextEntry; ++entryIdx)
    {
        const RecurringEntry & entry = mRecurringEntries[entryIdx];

        // Important:
        // We cannot simply compare the Entry Identity the Identity of a section to test if the Entry belongs there:
        // the parent index could be different (i.e. parents are distinct entries),
        // yet both parents could still belong to the same section.
        // For this reason, we actually have to lookup which section each parent belongs to,
        // this is the purpose of the lookup below.
        if(auto found = std::find_if(sections.begin(), sections.end(),
                                     [&entry, &lookup = entryIdToLogicalSectionId](const auto & aCandidateSection)
                                     {
                                        assert(entry.mId.mParentIdx == RecurringEntry::gNoParent 
                                            || (lookup[entry.mId.mParentIdx] != gInvalidLogicalSection));

                                        return aCandidateSection.mId.mName == entry.mId.mName
                                                // Check if the entry and the candidate section have the same logical parent:
                                                // * either both have a parent, then we compare the LogicalSectionId of both parents
                                                // * OR both have no parent
                                            && (   (   entry.mId.mParentIdx != RecurringEntry::gNoParent 
                                                    && aCandidateSection.mId.mParentIdx != RecurringEntry::gNoParent 
                                                    && lookup[entry.mId.mParentIdx] == lookup[aCandidateSection.mId.mParentIdx])
                                                || (   entry.mId.mParentIdx == RecurringEntry::gNoParent
                                                    && aCandidateSection.mId.mParentIdx == RecurringEntry::gNoParent))
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

    // Accumulate the values accounted for by child sections.
    for(LogicalSection & section : sections)
    {
        if(section.mId.mParentIdx != RecurringEntry::gNoParent)
        {
            LogicalSection & parentSection = sections[entryIdToLogicalSectionId[section.mId.mParentIdx]];
            parentSection.accountChildSection(section);
        }
    }

    auto sortedTime = Clock::now();

    //
    // Write the aggregated results to output stream
    //
    for(const LogicalSection & section : sections)
    {
        aOut << std::string(section.mId.mLevel * 2, ' ') << section.mId.mName << ":" 
            ;
        
        for (std::size_t valueIdx = 0; valueIdx != section.mActiveMetrics; ++valueIdx)
        {
            const auto & metric = section.mValues[valueIdx];
            const ProviderInterface & provider = getProvider(metric);

            aOut << " " << provider.mQuantityName << " " 
                << provider.scale(metric.mValues.average())
                // Show what has not been accounted for by child sections in between parenthesis
                << " (" << provider.scale(metric.mValues.average() - section.mAccountedFor[valueIdx]) << ")"
                << " " << provider.mUnit << ","
                ;
        }

        if(section.mEntries.size() > 1)
        {
            aOut << " (accumulated: " << section.mEntries.size() << ")";
        }

        aOut << "\n";
    }

    //
    // Output timing statistics about this function itself
    //
    {
        using std::chrono::microseconds;
        aOut << "(generated from "
            << mFrameState.mNextEntry << " entries in " 
            << getTicks<microseconds>(Clock::now() - beginTime) << " us"
            << ", sort took " << getTicks<microseconds>(sortedTime - beginTime) << " us"
            << ")"
            ;
    }
}


} // namespace ad::renderer