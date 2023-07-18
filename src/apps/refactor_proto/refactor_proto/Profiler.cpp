#include "Profiler.h"

#include "Logging.h"

#include <cassert>


namespace ad::renderer {


Profiler::Profiler()
{
    mMetricProviders.push_back(std::make_unique<ProviderCPUTime>());
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
        for (std::size_t i = 0; i != entry.mActiveMetrics; ++i)
        {
            if(getProvider(entry.mMetrics[i]).provide(i, queryFrame, result))
            {
                entry.mMetrics[i].mValues.record(result);
            }
            else
            {
                SELOG(error)("A provider result was not available when queried, it is discarded.");
            }
        }
    }
}

Profiler::EntryIndex Profiler::beginSection(const char * aName, std::initializer_list<ProviderIndex> aProvider)
{
    // TODO Section identity is much more subtle than that
    // It should be robust to changing the structure of sections (loop variance, and logical structure)
    // and handle change in providers somehow.

    Entry & entry = mEntries[mNextEntry];
    entry.mLevel = mCurrentLevel++;
    entry.mName = aName;
    if(entry.mActiveMetrics == 0)
    {
        for(const ProviderIndex providerIndex : aProvider)
        {
            entry.mMetrics.at(entry.mActiveMetrics++).mProviderIndex = providerIndex;
        }
    }

    assert(aProvider.size() > 0 && entry.mActiveMetrics == mProviders.size());

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
    --mCurrentLevel;
}


std::string Profiler::prettyPrint() const
{
    std::ostringstream oss;

    for(EntryIndex i = 0; i != mNextEntry; ++i)
    {
        const Entry & entry = mEntries[i];

        oss << std::string(entry.mLevel * 2, ' ') << entry.mName << ":" 
            ;
        
        for (std::size_t i = 0; i != entry.mActiveMetrics; ++i)
        {
            const ProviderInterface & provider = getProvider(entry.mMetrics[i]);
            oss << " " << provider.mQuantityName << " " 
                << entry.mMetrics[i].mValues.average() 
                << " " << provider.mUnit << ","
                ;
        }

        oss << "\n";
    }

    return oss.str();
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