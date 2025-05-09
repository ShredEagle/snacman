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

        unsigned int mBufferBindCount{0};
        unsigned int mDrawCount{0};
        unsigned int mFboAttachCount{0};
        Memory mBufferMemory;
        Memory mTextureMemory;
    };

    void BindBuffer(GLenum target, GLuint buffer);
    void BufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
    void BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
    void Clear(GLbitfield mask);
    void CompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);
    void CompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
    void CompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
    void CompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
    void Disable(GLenum cap);
    void DrawArrays(GLenum mode, GLint first, GLsizei count);
    void DrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
    void DrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance);
    void DrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices);
    void DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
    void DrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance);
    void DrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex);
    void DrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance);
    void Enable(GLenum cap);
    void FramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level);
    void FramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
    void FramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
    void MultiDrawArraysIndirect(GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride);
    void MultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride);
    void PolygonMode(GLenum face, GLenum mode);
    void TexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
    void TexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
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
inline void GlApi::BindBuffer(GLenum target, GLuint buffer)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mBufferBindCount;
#endif
    glBindBuffer(target, buffer);
}


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
    // TODO: instrument
    glClear(mask);
}


inline void GlApi::Disable(GLenum cap)
{
    // TODO: instrument
    glDisable(cap);
}


inline void GlApi::Enable(GLenum cap)
{
    // TODO: instrument
    glEnable(cap);
}


inline void GlApi::FramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mFboAttachCount;
#endif
    return glFramebufferTexture(target, attachment, texture, level);
}


inline void GlApi::FramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mFboAttachCount;
#endif
    return glFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset);
}


inline void GlApi::FramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mFboAttachCount;
#endif
    return glFramebufferTextureLayer(target, attachment, texture, level, layer);
}


inline void GlApi::DrawArrays(GLenum mode, GLint first, GLsizei count)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mDrawCount;
#endif
    return glDrawArrays(mode, first, count); 
}


inline void GlApi::DrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mDrawCount;
#endif
    return glDrawArraysInstanced(mode, first, count, instancecount); 
}


inline void GlApi::DrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mDrawCount;
#endif
    return glDrawArraysInstancedBaseInstance(mode, first, count, instancecount, baseinstance); 
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


inline void GlApi::DrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mDrawCount;
#endif
    return glDrawElementsInstancedBaseInstance(mode, count, type, indices, instancecount, baseinstance);
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


inline void GlApi::MultiDrawArraysIndirect(GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mDrawCount;
#endif
    return glMultiDrawArraysIndirect(mode, indirect, drawcount, stride);
}


inline void GlApi::MultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride)
{
#if defined(SE_INSTRUMENT_GL)
    ++v().mDrawCount;
#endif
    return glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride);
}


inline void GlApi::PolygonMode(GLenum face, GLenum mode)
{
    // TODO instrument
    glPolygonMode(face, mode);
}


inline void GlApi::TexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels)
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
    v().mTextureMemory.mAllocated += (w * h * d * bitsPerPixel) / 8;
#endif
    glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
}


inline void GlApi::TexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
#if defined(SE_INSTRUMENT_GL)
    // Only gives an approximation at the moment
    // TODO Ad 2023/10/24: Is it actually how the reserved memory is computed?
    // Do we need to take into account some further alignment constraints?
    // How is byte rounding implemeneted?
    GLuint bitsPerPixel = graphics::getPixelFormatBitSize(internalformat);
    GLsizei w = width;
    GLsizei h = height;
    for (GLsizei i = 0; i < levels; i++) {
        // Currently rounding each level independently.
        v().mTextureMemory.mAllocated += (w * h * bitsPerPixel) / 8;
        w = std::max(1, (w / 2));
        h = std::max(1, (h / 2));
    }
#endif
    return glTexStorage2D(target, levels, internalformat, width, height);
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


inline void GlApi::CompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data)
{
#if defined(SE_INSTRUMENT_GL)
    v().mTextureMemory.mWritten += imageSize;
#endif
    return glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}


inline void GlApi::CompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data)
{
#if defined(SE_INSTRUMENT_GL)
    v().mTextureMemory.mWritten += imageSize;
#endif
    return glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}


inline void GlApi::CompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data)
{
#if defined(SE_INSTRUMENT_GL)
    v().mTextureMemory.mWritten += imageSize;
#endif
    return glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
}


inline void GlApi::CompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data)
{
#if defined(SE_INSTRUMENT_GL)
    v().mTextureMemory.mWritten += imageSize;
#endif
    return glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}


} // namespace ad::renderer