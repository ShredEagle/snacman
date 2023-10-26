#include "ProfilerSeattle.h"

#include <cassert>


namespace ad::renderer {


struct LogicalSectionSeattle
{
    LogicalSectionSeattle(const ProfilerSeattle::Entry & aEntry, ProfilerSeattle::EntryIndex aEntryIndex) :
        mId{aEntry.mId},
        mValue{aEntry.mCpuTime},
        mEntries{aEntryIndex}
    {}

    void push(const ProfilerSeattle::Entry & aEntry, ProfilerSeattle::EntryIndex aEntryIndex)
    {
        mValue.mSingleValue += aEntry.mCpuTime.mSingleValue;
        mEntries.push_back(aEntryIndex);
    }

    void accountChildSection(const LogicalSectionSeattle & aNested)
    {
        mAccountedFor += aNested.mValue.average();
    }

    /// @brief Identity that determines the belonging of an Entry to this logical Section.
    ProfilerSeattle::Entry::Identity mId;

    // TODO should we use an array? 
    // Is it really useful to keep if we cumulate at push? (might be if we cache the sort result between frames)
    /// @brief Collection of entries beloging to this logical section. 
    std::vector<ProfilerSeattle::EntryIndex> mEntries;

    // TODO we actually only need to store Values here (not even sure we should keep the individual samples)
    // (note: we do not accumulate the samples at the moment)
    // If we go this route, we should split Metrics from AOS to SOA so we can copy Values from Entries with memcpy
    ProfilerSeattle::Value<ProfilerSeattle::Time_t> mValue;

    /// @brief Keep track of the samples accounted for by sub-sections 
    /// (thus allowing to know what has not been accounted for)
    ProfilerSeattle::Time_t mAccountedFor{0}; 
};


void ProfilerSeattle::prettyPrint(std::ostream & aOut) const
{
    // Measure end time, resulting in the time difference to be stored.
    Time_t beginTime = getTicks();

    //
    // Group by identity
    //

    // TODO Ad 2023/08/03: #sectionid Since for the moment we expect the "frame entry structure" to stay identical
    // we actually do not need to sort again at each frame.
    // In the long run though, we probably want to handle frame structure changes,
    // but there might be an optimization when we can guarantee the structure is identical to previous frame.
    std::vector<LogicalSectionSeattle> sections; // Note: We know there are at most mNextEntry logical sections,
                                          // might allow for a preallocated array

    using LogicalSectionId = unsigned short; // we should not have that many logical sections as to require a ushort, right?
    static constexpr LogicalSectionId gInvalidLogicalSection = std::numeric_limits<LogicalSectionId>::max();
    // This vector associate each entry index to a logical section index.
    // It allows to test whether distinct entry indices correspond to the same logical section, for parent consolidation.
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

    // Accumulate the values accounted for by child sections.
    for(LogicalSectionSeattle & section : sections)
    {
        if(section.mId.mParentIdx != Entry::gNoParent)
        {
            LogicalSectionSeattle & parentSection = sections[entryIdToLogicalSectionId[section.mId.mParentIdx]];
            parentSection.accountChildSection(section);
        }
    }

    Time_t sortedTime = getTicks();

    //
    // Write the aggregated results to output stream
    //
    static const std::string gUnit = "us";
    for(const LogicalSectionSeattle & section : sections)
    {
        aOut << std::string(section.mId.mLevel * 2, ' ') << section.mId.mName << ":" 
            ;
        
        const auto & value = section.mValue;
        aOut << " " << "Cpu time "
            << toMicro(value.average())
            // Show what has not been accounted for by child sections in between parenthesis
            << " (" << toMicro(value.average() - section.mAccountedFor) << ")"
            << " " << gUnit << ","
            ;

        if(section.mEntries.size() > 1)
        {
            aOut << " (accumulated: " << section.mEntries.size() << ")";
        }

        aOut << "\n";
    }

    //
    // Output timing statistics about this function itself
    //
    Time_t now = getTicks();
    {
        aOut << "(generated from "
            << mNextEntry << " entries in " 
            << toMicro(now - beginTime) << " " << gUnit
            << ", sort took " << toMicro(sortedTime - beginTime) << " " << gUnit
            << ")"
            ;
    }
}


} // namespace ad::renderer