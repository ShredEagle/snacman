#pragma once


#include "../Profiler.h"

#include <renderer/Query.h> 


namespace ad::renderer {


struct ProviderGL : public ProviderInterface
{
    ProviderGL() :
        ProviderInterface{"GPU generated", "primitive(s)"}
    {}

    void beginSectionRecurring(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;
    void endSectionRecurring(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;

    bool provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult) override;

    void resize(std::size_t aNewEntriesCount) override
    { mQueriesPool.resize(aNewEntriesCount * Profiler::CountSubframes() ); }

    //////

    graphics::Query & getQuery(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame);

    std::vector<graphics::Query> mQueriesPool;
    bool mActive{false}; // There can be at most 1 query active per target (per index, for indexed targets)
};


struct ProviderGLTime : public ProviderInterface
{
    enum class Event
    {
        Begin,
        End,
    };

    ProviderGLTime() :
        ProviderInterface{"GPU time", "us"}
    {}

    void beginSectionRecurring(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;
    void endSectionRecurring(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override;

    bool provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult) override;

    // This is currently very conservative, having enough queries to recorded begin/end time for each section
    // in each "frame delay slot".
    void resize(std::size_t aNewEntriesCount) override
    { mQueriesPool.resize(2 * aNewEntriesCount * Profiler::CountSubframes() ); }

    //////
    graphics::Query & getQuery(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame, Event aEvent);

    std::vector<graphics::Query> mQueriesPool;
};


} // namespace ad::renderer