#include "Profiler.h"

#include "Logging.h"

#include <cassert>


namespace ad::renderer {


Profiler::Profiler() :
    mDurationsProvider{std::make_unique<ProviderCPUTime>()},
    mPrimitivesProvider{std::make_unique<ProviderGL>()}
{}

void Profiler::beginFrame()
{
    ++mFrameNumber;
    mNextEntry = 0;
    mCurrentLevel = 0; // should already be the case
}

void Profiler::endFrame()
{
    assert(mCurrentLevel == 0); // sanity check

    // Ensure we have a full circular buffer (of size gFrameDelay)
    if((mFrameNumber + 1) < gFrameDelay)
    {
        return;
    }

    for(EntryIndex i = 0; i != mNextEntry; ++i)
    {
        Entry & entry = mEntries[i];
        std::uint32_t queryFrame = (mFrameNumber + 1) % gFrameDelay;

        GLuint result;
        if(mPrimitivesProvider->provide(i, queryFrame, result))
        {
            entry.mPrimitivesGenerated.mValues.record(result);
        }
        else
        {
            SELOG(error)("An OpenGL Query result was not available, it is discarded.");
        }

        mDurationsProvider->provide(i, queryFrame, result);
        entry.mCpuDurations.mValues.record(result);
    }
}

Profiler::EntryIndex Profiler::beginSection(const char * aName)
{
    Entry & entry = mEntries[mNextEntry];
    entry.mLevel = mCurrentLevel++;
    entry.mName = aName;

    mDurationsProvider->beginSection(mNextEntry, mFrameNumber % gFrameDelay);
    mPrimitivesProvider->beginSection(mNextEntry, mFrameNumber % gFrameDelay);

    return mNextEntry++;
}


void Profiler::endSection(EntryIndex aIndex)
{
    mDurationsProvider->endSection(aIndex, mFrameNumber % gFrameDelay);
    mPrimitivesProvider->endSection(aIndex, mFrameNumber % gFrameDelay);
    --mCurrentLevel;
}


std::string Profiler::prettyPrint() const
{
    std::ostringstream oss;

    for(EntryIndex i = 0; i != mNextEntry; ++i)
    {
        const Entry & entry = mEntries[i];

        oss << std::string(entry.mLevel * 2, ' ') 
            << entry.mName << ": " << entry.mCpuDurations.mValues.average() << " µs, "  
            << entry.mPrimitivesGenerated.mValues.average() << " primitives generated.\n"
            ;
    }

    return oss.str();
}


ProviderGL::ProviderGL() :
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


bool ProviderGL::provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult)
{
    auto & query = getQuery(aEntryIndex, aQueryFrame);

    GLuint available;
    glGetQueryObjectuiv(query, GL_QUERY_RESULT_AVAILABLE, &available);

    if(available)
    {
        glGetQueryObjectuiv(query, GL_QUERY_RESULT, &aSampleResult);
    }

    return available != GL_FALSE;
}


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
    using std::chrono::microseconds;
    const auto & interval = getInterval(aEntryIndex, aQueryFrame);
    // TODO address this cast
    aSampleResult = (GLuint)std::chrono::duration_cast<microseconds>(interval.mEnd - interval.mBegin).count();
    return true;
}


} // namespace ad::renderer