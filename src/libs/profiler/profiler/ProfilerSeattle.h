#pragma once


#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>


#include <array>
#include <limits>
#include <ostream>
#include <vector>

#include <cassert>
#include <cstdint>


namespace ad::renderer {


inline std::int64_t getTicks()
{
    LARGE_INTEGER tmp;
    QueryPerformanceCounter(&tmp);
    return tmp.QuadPart;
}


inline std::int64_t getTickFrequency()
{
    LARGE_INTEGER tmp;
    QueryPerformanceFrequency(&tmp);
    return tmp.QuadPart;
}


/// @brief Provide a CPU timer with the best performance we can achieve, at the cost of flexibility.
class ProfilerSeattle
{
    friend struct LogicalSectionSeattle; // Implementation detail for grouping entries by logical section.

public:
    using Time_t = std::int64_t;
    inline static constexpr std::size_t gInitialEntries{512};

    using EntryIndex = std::size_t;

    ProfilerSeattle() :
        mTickPeriodNano{Time_t(1E9)/getTickFrequency()}
    {
        if((Time_t(1E9) % getTickFrequency()) != 0)
        {
            throw std::logic_error{"Period is not an exact integer."};
        }
        mEntries.resize(gInitialEntries);
    }


    void beginFrame();
    void endFrame();

    EntryIndex beginSection(const char * aName, std::initializer_list<std::size_t>);
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

        ProfilerSeattle * mProfiler;
        EntryIndex mEntry;
    };
    
    SectionGuard scopeSection(const char * aName, std::initializer_list<std::size_t> aDummy)
    { 
        return {
            .mProfiler = this,
            .mEntry = beginSection(aName, aDummy)
        };
    }

    void prettyPrint(std::ostream & aOut) const;

    template <class T_value>
    struct Value
    {
        //void record(T_value aSample)
        //{ mSingleValue = aSample }

        T_value average() const
        { return mSingleValue; }

        T_value mSingleValue;
    };

    Time_t toMicro(Time_t aTicks) const
    { return aTicks * mTickPeriodNano / 1000; }

private:
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
        Value<Time_t> mCpuTime;
    };

    /// @brief Returns the next entry, handling resizing of storage when needed.
    Entry & getNextEntry();

    void resize(std::size_t aNewEntriesCount);

    Time_t mTickPeriodNano;
    std::vector<Entry> mEntries; // Initial size handled in constructor
    EntryIndex mNextEntry{0};
    unsigned int mCurrentLevel{0};
    EntryIndex mCurrentParent{Entry::gNoParent};
    unsigned int mFrameNumber{std::numeric_limits<unsigned int>::max()};
};


inline void ProfilerSeattle::beginFrame()
{
    ++mFrameNumber;
    mNextEntry = 0;
    mCurrentLevel = 0; // should already be the case
    mCurrentParent = Entry::gNoParent; // should already be the case
}


inline void ProfilerSeattle::endFrame()
{
    assert(mCurrentLevel == 0); // sanity check
    assert(mCurrentParent == Entry::gNoParent); // sanity check
}


inline ProfilerSeattle::Entry & ProfilerSeattle::getNextEntry()
{
    if(mNextEntry == mEntries.size())
    {
        auto newSize = mEntries.size() * 2;
        mEntries.resize(newSize);
    }
    return mEntries[mNextEntry];
}


inline ProfilerSeattle::EntryIndex ProfilerSeattle::beginSection(const char * aName, std::initializer_list<std::size_t>)
{
    Entry & entry = getNextEntry();

    // Because we do not track identity properly when frame structure change, we hope it always matches
    assert(entry.mId.mName == nullptr || entry.mId.mName == aName);
    assert(entry.mId.mLevel == Entry::gInvalidLevel || entry.mId.mLevel == mCurrentLevel);
    assert(entry.mId.mParentIdx == Entry::gInvalidEntry || entry.mId.mParentIdx == mCurrentParent);

    entry.mId.mName = aName;
    entry.mId.mLevel = mCurrentLevel++;
    entry.mId.mParentIdx = mCurrentParent;
    mCurrentParent = mNextEntry; //i.e. this Entry index becomes the current parent

    // Measure begin time
    entry.mCpuTime.mSingleValue = -getTicks();

    return mNextEntry++;
}


inline void ProfilerSeattle::endSection(EntryIndex aIndex)
{
    Entry & entry = mEntries[aIndex];
    // Measure end time, resulting in the time difference to be stored.
    entry.mCpuTime.mSingleValue += getTicks();

    mCurrentParent = entry.mId.mParentIdx; // restore this Entry parent as the current parent
    --mCurrentLevel;
}


inline void ProfilerSeattle::popCurrentSection()
{
    endSection(mCurrentParent);
}


} // namespace ad::renderer