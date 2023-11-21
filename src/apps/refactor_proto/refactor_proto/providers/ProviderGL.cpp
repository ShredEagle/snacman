#include "ProviderGL.h"

#include <cassert>


namespace ad::renderer {



graphics::Query & ProviderGL::getQuery(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    return mQueriesPool[aEntryIndex * Profiler::CountSubframes()  + aCurrentFrame];
}


void ProviderGL::beginSectionRecurring(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    assert(!mActive);
    mActive = true;
    glBeginQueryIndexed(GL_PRIMITIVES_GENERATED, 0, getQuery(aEntryIndex, aCurrentFrame));
}


void ProviderGL::endSectionRecurring(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
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
    return mQueriesPool[2 * (aEntryIndex * Profiler::CountSubframes()  + aCurrentFrame) 
                        + (aEvent == Event::Begin ? 0 : 1)];
}

void ProviderGLTime::beginSectionRecurring(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
{
    glQueryCounter(getQuery(aEntryIndex, aCurrentFrame, Event::Begin), GL_TIMESTAMP);
}


void ProviderGLTime::endSectionRecurring(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
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