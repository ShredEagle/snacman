#include "Profiler.h"

#include "Logging.h"


#include "providers/ProviderApi.h"
#include "providers/ProviderGL.h"
#include "providers/ProviderRdtsc.h"

#include <utilities/Time.h>


#if defined(_WIN32)
//#include "providers/ProviderWindows.h"
#endif

#include <cassert>


namespace ad::renderer {


bool Profiler::Entry::matchIdentity(const char * aName, const FrameState & aFrameState) const
{
    return 
        (mId.mName == aName)
        && (mId.mLevel == aFrameState.mCurrentLevel)
        && (mId.mParentIdx == aFrameState.mCurrentParent)
    ;
}


template <std::forward_iterator T_iterator>
bool Profiler::Entry::matchProviders(T_iterator aFirstProviderIdx, T_iterator aLastProviderIdx) const
{
    return std::equal(mMetrics.begin(), mMetrics.begin() + mActiveMetrics,
                      aFirstProviderIdx, aLastProviderIdx,
                      [](const Profiler::Metric<Profiler::Sample_t> & aMetric, Profiler::ProviderIndex aProviderIdx)
                      {
                          return aMetric.mProviderIndex == aProviderIdx;
                      });
}


template <std::forward_iterator T_iterator>
void Profiler::Entry::setProviders(T_iterator aFirstProviderIdx, T_iterator aLastProviderIdx)
{
    mActiveMetrics = 0;
    [[maybe_unused]] ProviderIndex previousProvider = std::numeric_limits<ProviderIndex>::max(); // only used for the sorting assertion
    //for(const ProviderIndex providerIndex : aProviders)
    for(auto providerIdxIt = aFirstProviderIdx; providerIdxIt != aLastProviderIdx; ++providerIdxIt)
    {
        ProviderIndex providerIndex = *providerIdxIt;
        // Provider indices must always be sorted (this is notably an assumption in LogicalSection::accountForChildSection)
        assert(previousProvider == std::numeric_limits<ProviderIndex>::max() || previousProvider < providerIndex
            && "Provider indices must given in ascending order.");
        previousProvider = providerIndex;

        mMetrics.at(mActiveMetrics++) = Metric<Sample_t>{
            // TODO we could optimize that without rewritting each individual sample to zero
            // Note: this could be more tricky with the introduction of single shot
            .mProviderIndex = providerIndex,
            .mValues = {}, // Note: No effect line, to make explicit that we reset values.
        };
    }
}


void Profiler::Entry::resetValues()
{
    for (std::size_t metricIdx = 0; metricIdx != mActiveMetrics; ++metricIdx)
    {
        mMetrics[metricIdx].mValues = {};
    }
}


Profiler::Profiler(Providers aProviderFeatures)
{
//#if defined(_WIN32)
//    mMetricProviders.push_back(std::make_unique<ProviderCpuPerformanceCounter>());
//#else
    mMetricProviders.push_back(std::make_unique<ProviderCpuRdtsc>());
    //mMetricProviders.push_back(std::make_unique<ProviderCPUTime>());
    if(aProviderFeatures == Providers::All)
    {
        mMetricProviders.push_back(std::make_unique<ProviderGLTime>());
        mMetricProviders.push_back(std::make_unique<ProviderGL>());
        mMetricProviders.push_back(std::make_unique<ProviderApi<&GlApi::Metrics::drawCount>>("draw", ""));
        mMetricProviders.push_back(std::make_unique<ProviderApi<&GlApi::Metrics::bufferMemoryWritten>>("buffer w", "B"));
    }

    resize(gInitialEntries);
}


Profiler::EntryIndex Profiler::fetchNextEntry(EntryNature aNature, const char * aName)
{
    // Need to the actual subframe to fill the "fetchedSubframe" of a single shot.
    std::uint32_t actualSubframe = currentSubframe(EntryNature::Recurring);
    if(aNature == EntryNature::SingleShot)
    {
        // check if an existing single shot entry with this name is already present
        for(auto & [existingIdx, fetchedSubframe] : mSingleShots)
        {
            Entry & entry = mEntries[existingIdx];
            // TODO should we compare on the name content, instead of just the name ptr?
            if(entry.mId.mName == aName)
            {
                // Already in the single shots list
                fetchedSubframe = actualSubframe;
                return existingIdx;
            }
        }
    }

    //
    // Common path for any recurring entry as well as a *new* single shot
    // (an existing single shot would already have returned)
    //
    while (mFrameState.mNextEntry != mEntries.size() 
           && mEntries[mFrameState.mNextEntry].isSingleShot())
    {
        ++mFrameState.mNextEntry;
    }

    // Resize if the selected idx is out of bounds
    if(mFrameState.mNextEntry == mEntries.size())
    {
        auto newSize = mEntries.size() * 2;
        SELOG(debug)("Resizing the profiler to {} entries.", newSize);
        resize(newSize);
    }

    if(aNature == EntryNature::SingleShot)
    {
        mSingleShots.emplace_back(mFrameState.mNextEntry, actualSubframe);
    }
    else
    {
        mFrameState.mRecurrings.push_back(mFrameState.mNextEntry);
    }

    return mFrameState.mNextEntry++;
}


// Note: Would be better as a member function of Entry, but we need the index, which is not stored in the Entry
void Profiler::queryProviders(EntryIndex aEntryIdx, std::uint32_t aQueryFrame)
{
    Sample_t result;
    Entry & entry = mEntries[aEntryIdx];
    for (std::size_t metricIdx = 0; metricIdx != entry.mActiveMetrics; ++metricIdx)
    {
        if(getProvider(entry.mMetrics[metricIdx]).provide(aEntryIdx, aQueryFrame, result))
        {
            entry.mMetrics[metricIdx].mValues.record(result);
        }
        else
        {
            SELOG(error)("A provider result was not available when queried, it is discarded.");
        }
    }
}


void Profiler::resize(std::size_t aNewEntriesCount)
{
    mEntries.resize(aNewEntriesCount);
    for(const auto & provider : mMetricProviders)
    {
        provider->resize(aNewEntriesCount);
    }
}


ProviderInterface & Profiler::getProvider(const Metric<Sample_t> & aMetric)
{
    return *mMetricProviders.at(aMetric.mProviderIndex);
}


const ProviderInterface & Profiler::getProvider(const Metric<Sample_t> & aMetric) const
{
    return *mMetricProviders.at(aMetric.mProviderIndex);
}


std::uint32_t Profiler::currentSubframe(EntryNature aNature) const
{
    if (aNature == EntryNature::SingleShot)
    {
        return Entry::gSingleShotSubframe;
    }
    else
    {
        return mFrameState.mFrameNumber % CountSubframes();
    }
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
    assert(mEnabled 
        && "The disable mechanism is only intended to inhibit sections of frame profiling.");
    assert(mFrameState.areAllSectionsClosed());

    // TODO Ad 2023/11/15: I do not like this explicit reset of all values at the end of the frame if a reset occurred:
    // this is wasteful of resources, since all values for entities that changed have already been reset
    // But this is currently imposed by the prettyPrint, which expects all Entities of a logical section to have the same count of samples...
    if(mFrameState.mFrameNumber == mLastResetFrame)
    {
        // If a reset occured during this frame, reset all values in all recurring entries
        for(EntryIndex entryIdx : mFrameState.mRecurrings)
        {
            Entry & entry = mEntries[entryIdx];
            entry.resetValues();
        }
    }

    std::uint32_t subframeToQuery = queriedSubframe();

    for(auto & [entryIdx, fetchedSubframe] : mSingleShots)
    {
        if(fetchedSubframe == subframeToQuery)
        {
            // There could be previous values if this single shot was reused,
            // or if it was fetched at an index previously containing a recurring entry.
            mEntries[entryIdx].resetValues(); // Erase any previous value.
            queryProviders(entryIdx, Entry::gSingleShotSubframe);
            fetchedSubframe = -1; // do not query again, this subframe will not occur
        }
    }

    // Ensure we have enough entries in the circular buffer before issuing queries (frame count since reset >= gFrameDelay)
    if(mFrameState.mFrameNumber < (mLastResetFrame + gFrameDelay))
    {
        return;
    }

    for(EntryIndex entryIdx : mFrameState.mRecurrings)
    {
        queryProviders(entryIdx, subframeToQuery);
    }
}


Profiler::EntryIndex Profiler::beginSection(EntryNature aNature, const char * aName, std::initializer_list<ProviderIndex> aProviders)
{
    if(!mEnabled)
    {
        return Entry::gInvalidEntry;
    }
    
    // TODO Section identity is much more subtle than that
    // It should be robust to changing the structure of sections (loop variance, and logical structure)
    // and handle change in providers somehow.
    // For the moment, make a few assertions that will trip the program as soon as structure changes.

    // There must be a number of provider in [1, maxMetricsPerSection]
    assert(aProviders.size() > 0 && aProviders.size() <= gMaxMetricsPerSection);
    
    EntryIndex entryIndex = std::numeric_limits<EntryIndex>::max();
    if(aNature == EntryNature::Recurring)
    {
        bool alreadyPresent;
        std::tie(entryIndex, alreadyPresent) = setupNextEntryRecurring(aName, aProviders);

        if (!alreadyPresent)
        {
            mLastResetFrame = mFrameState.mFrameNumber;
            SELOG(debug)("Profiler sections structure changed.");
        }
    }
    else
    {
        assert(aNature == EntryNature::SingleShot);
        entryIndex = setupNextEntrySingleShot(aName, aProviders);
    }

    Entry & entry = mEntries[entryIndex];
    std::uint32_t subframe = currentSubframe(aNature);
    for (std::size_t i = 0; i != entry.mActiveMetrics; ++i)
    {
        getProvider(entry.mMetrics[i]).beginSection(entryIndex, subframe);
    }

    return entryIndex;
}


void Profiler::endSection(EntryIndex aIndex)
{
    if(!mEnabled)
    {
        // Otherwise, there is likely a mismatch in disabled sections
        assert(aIndex == Entry::gInvalidEntry);
        return;
    }

    Entry & entry = mEntries.at(aIndex);
    std::uint32_t subframe = currentSubframe(entry.getNature());
    for (std::size_t i = 0; i != entry.mActiveMetrics; ++i)
    {
        getProvider(entry.mMetrics[i]).endSection(aIndex, subframe);
    }

    if(entry.getNature() == EntryNature::Recurring)
    {
        mFrameState.mCurrentParent = entry.mId.mParentIdx; // restore this Entry parent as the current parent
        --mFrameState.mCurrentLevel;
    }
}


void Profiler::popCurrentSection()
{
    endSection(mFrameState.mCurrentParent);
}


std::pair<Profiler::EntryIndex, bool> Profiler::setupNextEntryRecurring(const char * aName, auto aProviders)
{
    EntryIndex entryIdx = fetchNextEntry(EntryNature::Recurring, aName/*unused*/);
    Entry & entry = mEntries[entryIdx];
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

        entry.setProviders(aProviders.begin(), aProviders.end());
    }

    ++mFrameState.mCurrentLevel;
    mFrameState.mCurrentParent = entryIdx;
    return {entryIdx, alreadyPresent};
}


Profiler::EntryIndex Profiler::setupNextEntrySingleShot(const char * aName, auto aProviders)
{
    EntryIndex entryIdx = fetchNextEntry(EntryNature::SingleShot, aName);
    Entry & entry = mEntries[entryIdx];

    entry.mId.mName = aName;
    entry.mId.mLevel = Entry::gSingleShotLevel;

    entry.setProviders(aProviders.begin(), aProviders.end());

    return entryIdx;
}


struct LogicalSection
{
    LogicalSection(const Profiler::Entry & aEntry, Profiler::EntryIndex aEntryIndex) :
        mId{aEntry.mId},
        mEntries{aEntryIndex},
        mValues{aEntry.mMetrics},
        mActiveMetrics{aEntry.mActiveMetrics}
    {}

    void push(const Profiler::Entry & aEntry, Profiler::EntryIndex aEntryIndex)
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
                ++mAccountedChildren[thisMetricIdx];
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

    /// @brief Returns wether this LogicalSection has accounted children samples for metric `aMetrixIdx`.
    /// (Otherwise, it is expected that the accounted total will be zero, providing no valuable information for display)
    bool hasAccountedChildrenFor(std::size_t aMetricIdx) const
    { 
        return mAccountedChildren[aMetricIdx] != 0; 
    }

    /// @brief Identity that determines the belonging of an Entry to this logical Section.
    Profiler::Entry::Identity mId;
    // TODO should we use an array? 
    // Is it really useful to keep if we cumulate at push? (might be if we cache the sort result between frames)
    /// @brief Collection of entries beloging to this logical section. 
    std::vector<Profiler::EntryIndex> mEntries;

    // TODO we actually only need to store Values here (not even sure we should keep the individual samples)
    // (note: we do not accumulate the samples at the moment)
    // If we go this route, we should split Metrics from AOS to SOA so we can copy Values from Entries with memcpy
    std::array<Profiler::Metric<Profiler::Sample_t>, Profiler::gMaxMetricsPerSection> mValues;
    std::size_t mActiveMetrics = 0;

    /// @brief Keep track of the samples accounted for by sub-sections 
    /// (thus allowing to know what has not been accounted for)
    std::array<Profiler::Sample_t, Profiler::gMaxMetricsPerSection> mAccountedFor{}; // request value-initialization
    std::array<unsigned int, Profiler::gMaxMetricsPerSection> mAccountedChildren{};
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
    std::vector<LogicalSectionId> entryIdToLogicalSectionId(mEntries.size(), gInvalidLogicalSection);

    //for(EntryIndex entryIdx = 0; entryIdx != mFrameState.mNextEntry; ++entryIdx)
    for(EntryIndex entryIdx : mFrameState.mRecurrings)
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

    // Accumulate the values accounted for by child sections.
    for(LogicalSection & section : sections)
    {
        if(section.mId.mParentIdx != Entry::gNoParent)
        {
            LogicalSection & parentSection = sections[entryIdToLogicalSectionId[section.mId.mParentIdx]];
            parentSection.accountChildSection(section);
        }
    }

    auto sortedTime = Clock::now();

    auto printSection = [this](std::ostream & aOut, LogicalSection aSection)
    {
        aOut << std::string(aSection.mId.mLevel * 2, ' ') << aSection.mId.mName << ":" 
            ;
        
        for (std::size_t valueIdx = 0; valueIdx != aSection.mActiveMetrics; ++valueIdx)
        {
            const auto & metric = aSection.mValues[valueIdx];
            const ProviderInterface & provider = getProvider(metric);

            aOut << " " << provider.mQuantityName << " " 
                << provider.scale(metric.mValues.average())
                // Show what has not been accounted for by child sections in between parenthesis
                ;

            if(aSection.hasAccountedChildrenFor(valueIdx))
            {
                aOut << " (" << provider.scale(metric.mValues.average() - aSection.mAccountedFor[valueIdx]) << ")"
                    ;
            }

            aOut << " " << provider.mUnit << ","
                ;
        }

        if(aSection.mEntries.size() > 1)
        {
            aOut << " (accumulated: " << aSection.mEntries.size() << ")";
        }
    };

    auto printSingleEntry = [this](std::ostream & aOut, Entry aEntry)
    {
        aOut << aEntry.mId.mName << ":" 
            ;
        
        for (std::size_t valueIdx = 0; valueIdx != aEntry.mActiveMetrics; ++valueIdx)
        {
            const auto & metric = aEntry.mMetrics[valueIdx];
            const ProviderInterface & provider = getProvider(metric);

            aOut << " " << provider.mQuantityName << " " 
                << provider.scale(metric.mValues.average())
                << " " << provider.mUnit << ","
                ;
        }
    };

    //
    // Write the aggregated results to output stream
    //
    for(const LogicalSection & section : sections)
    {
        printSection(aOut, section);
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
            << ")\n"
            ;
    }

    //
    // Write the single shots to output stream
    // 
    aOut << "\n|| Single shots ||\n\n";
    for(EntryIndex entryIdx : mSingleShots)
    {
        printSingleEntry(aOut, mEntries[entryIdx]);
        aOut << "\n";
    }
}


} // namespace ad::renderer