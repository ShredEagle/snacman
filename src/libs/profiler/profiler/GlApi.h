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
        std::size_t mAllocated{0};
        std::size_t mWritten{0};
    };

    struct Metrics
    {
        unsigned int drawCount() const
        { return mDrawCount; }

        std::size_t bufferMemoryWritten() const
        { return mBufferMemory.mWritten ; }

        unsigned int mDrawCount{0};
        Memory mBufferMemory;
        Memory mTextureMemory;
    };

    void BufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
    void BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
    void Clear(GLbitfield mask);
    void DrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices);
    void DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
    void DrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex);
    void DrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance);
    void MultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride);
    void TexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
    void TexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);


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


inline void GlApi::DrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mDrawCount;
#endif
    return glDrawElements(mode, count, type, indices);
}


inline void GlApi::DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mDrawCount;
#endif
    return glDrawElementsInstanced(mode, count, type, indices, instancecount);
}


inline void GlApi::DrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mDrawCount;
#endif
    return glDrawElementsInstancedBaseVertex(mode, count, type, indices, instancecount, basevertex);
}


inline void GlApi::DrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mDrawCount;
#endif
    return glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, indices, instancecount, basevertex, baseinstance);
}


inline void GlApi::MultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mDrawCount;
#endif
    return glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride);
}


inline void GlApi::TexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
#if defined(SE_INSTRUMENT_GL)
    // Only gives an approximation at the moment
    // TODO Ad 2023/10/24: Is it actually how the reserved memory is computed?
    // Do we need to take into account some further alignment constraints?
    // How is byte rounding implemeneted?
    GLuint bitsPerPixel = graphics::getPixelFormatBitSize(internalformat);
    GLsizei w = width;
    GLsizei h = height;
    GLsizei d = depth;
    for (GLsizei i = 0; i < levels; i++) {
        // Currently rounding each level independently.
        v().mTextureMemory.mAllocated += (w * h * d * bitsPerPixel) / 8;
        w = std::max(1, (w / 2));
        h = std::max(1, (h / 2));
        if(target == GL_TEXTURE_3D || target == GL_PROXY_TEXTURE_3D)
        {
            d = std::max(1, (d / 2));
        }
    }
#endif
    return glTexStorage3D(target, levels, internalformat, width, height, depth);
}


inline void GlApi::TexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels)
{
#if defined(SE_INSTRUMENT_GL)
    // TODO Ad 2023/10/24: Handle packed pixel data types, such as GL_UNSIGNED_BYTE_3_3_2
    // see: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexSubImage3D.xhtml
    v().mTextureMemory.mWritten += 
        width * height * depth * graphics::getComponentsCount(format) * graphics::getByteSize(type);
#endif
    return glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}


} // namespace ad::renderer