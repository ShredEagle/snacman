#pragma once

#include <renderer/GL_Loader.h>
#include <renderer/MappedGL.h>

#include <array>

#include <cassert>
#include <cstddef>


namespace ad::renderer {


#define SE_INSTRUMENT_GL

class GlApi
{
public:
    struct Memory
    {
        std::size_t mAllocated;
        std::size_t mWritten;
    };

    struct Metrics
    {
        unsigned int drawCount() const
        { return mDrawCount; }

        std::size_t bufferMemoryWritten() const
        { return mBufferMemory.mWritten ; }

        unsigned int mDrawCount{0};
        Memory mBufferMemory{0};
    };

    void BufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
    void BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
    void Clear(GLbitfield mask);
    void MultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride);

    // Note: Give access to the Metrics even if the instrumentation is not macro enabled
    // (this way the reporting code can still compile)
    const Metrics & get() const
    {
        return mMetrics;
    }

private:
#if defined(SE_INSTRUMENT_GL)
    Metrics & v()
    { return mMetrics; }

    static constexpr std::size_t gMaxGlId = 1024;
    std::array<std::size_t, gMaxGlId> mAllocatedBufferMemory{0};
#endif

    Metrics mMetrics;

};


inline GlApi gl;


//
// Inline implementations    
//
inline void GlApi::BufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage)
{
#if defined(SE_INSTRUMENT_GL)
    GLint boundBuffer;
    glGetIntegerv(graphics::getGLMappedBufferBinding(target), &boundBuffer);
    assert(boundBuffer != 0); // note at() will catch out of bound access
    const std::size_t previousSize = mAllocatedBufferMemory.at(boundBuffer);
    mAllocatedBufferMemory.at(boundBuffer) = size;

    v().mBufferMemory.mAllocated -= previousSize;
    v().mBufferMemory.mAllocated += size;
    if(data)
    {
        v().mBufferMemory.mWritten += size;
    }

#endif
    return glBufferData(target, size, data, usage);
}


inline void GlApi::BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data)
{
#if defined(SE_INSTRUMENT_GL)
        v().mBufferMemory.mWritten += size;
#endif
    return glBufferSubData(target, offset, size, data);
}


inline void GlApi::Clear(GLbitfield mask)
{
    glClear(mask);
}


inline void GlApi::MultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mDrawCount;
#endif
    return glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride);
}


} // namespace ad::renderer