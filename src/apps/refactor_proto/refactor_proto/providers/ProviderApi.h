#pragma once


#include "../GlApi.h"
#include "../Profiler.h"


namespace ad::renderer {


template <auto F_getter>
// This alternative syntax captures the constraint in the non-type template parameter F_getter
// but forces the client to first explicitly specify the return value.
//template <class T_value, T_value(GlApi::Metrics::*F_getter)() const>
requires requires {(std::declval<GlApi::Metrics>().*F_getter)();}
struct ProviderApi : public ProviderInterface
{
    using Value_t = std::invoke_result_t<decltype(F_getter), GlApi::Metrics>;

    ProviderApi(const char * aQuantityName, const char * aUnit) :
        ProviderInterface{aQuantityName, aUnit}
    {}

    void beginSectionRecurring(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override
    {
        getRecord(aEntryIndex, aCurrentFrame) = (gl.get().*F_getter)();
    }

    void endSectionRecurring(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame) override
    {
        auto & count = getRecord(aEntryIndex, aCurrentFrame);
        count = (gl.get().*F_getter)() - count;
    }

    bool provide(EntryIndex aEntryIndex, uint32_t aQueryFrame, GLuint & aSampleResult) override
    {
        aSampleResult = (GLuint)getRecord(aEntryIndex, aQueryFrame);
        return true;
    }

    void resize(std::size_t aNewEntriesCount) override
    { mCounts.resize(aNewEntriesCount * Profiler::CountSubframes() ); }

    /////
    Value_t & getRecord(EntryIndex aEntryIndex, std::uint32_t aCurrentFrame)
    {
        return mCounts[aEntryIndex * Profiler::CountSubframes()  + aCurrentFrame];
    }

    std::vector<Value_t> mCounts;
};


} // namespace ad::renderer