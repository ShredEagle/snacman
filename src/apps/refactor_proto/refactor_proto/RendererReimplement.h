#pragma once


#include "GlApi.h"


namespace ad::renderer {


// TODO Ad 2023/10/18: We should not need to re-implement these.
// We need another approach:
// * renderer could be made aware of the GlApi wrapper (bad because of coupling, but easy).
// * understand how to insert our "custom gl wrapper" before glad.
//   This way we can instrument existing code, and renderer does not need to be aware.
//
// There is the major complication of how do we account for "discarded memory" with functions such
// as glBufferData (they allocate for non-created buffers, but "discard" previous memory for created buffers)
// How to get the current size of the created buffer without ruining performance, 
// while still allowing client code to directly use the (wrapped) gl functions (surch as glBufferData).

// Note: re-implement renderer lib functionalities that wrap gl calls, but with call to our custom gl abstraction
namespace proto {

    template <class T_data, std::size_t N_extent, graphics::BufferType N_type>
    void load(const graphics::Buffer<N_type> & aBuffer, std::span<T_data, N_extent> aData, graphics::BufferHint aUsageHint)
    {
        graphics::ScopedBind bound{aBuffer};
        gl.BufferData(static_cast<GLenum>(N_type),
                      aData.size_bytes(),
                      aData.data(),
                      getGLBufferHint(aUsageHint));
    }


    template <class T_data, std::size_t N_extent>
    void load(const graphics::BufferAny & aBuffer, std::span<T_data, N_extent> aData, graphics::BufferHint aUsageHint)
    {
        // TODO replace with DSA
        // Note: Waiting for DSA, use a random target, the underlying buffer objects are all identical.
        constexpr auto target = graphics::BufferType::Array;
        graphics::ScopedBind bound{aBuffer, target};
        gl.BufferData(static_cast<GLenum>(target),
                      aData.size_bytes(),
                      aData.data(),
                      getGLBufferHint(aUsageHint));
    }



    template <class T_data, graphics::BufferType N_type>
    void loadSingle(const graphics::Buffer<N_type> & aBuffer, T_data aInstanceData, graphics::BufferHint aUsageHint)
    {
        // avoid any ADL
        proto::load(aBuffer, std::span{&aInstanceData, 1}, aUsageHint);
    }


    template <class T_data>
    void loadSingle(const graphics::BufferAny & aBuffer, T_data aInstanceData, graphics::BufferHint aUsageHint)
    {
        proto::load(aBuffer, std::span{&aInstanceData, 1}, aUsageHint);
    }


    inline void writeTo(const graphics::Texture & aTexture,
                        const std::byte * aRawData,
                        const graphics::InputImageParameters & aInput,
                        math::Position<3, GLint> aTextureOffset,
                        GLint aMipmapLevelId = 0)
    {
        // TODO assert that texture target is of correct dimension.

        // TODO replace with DSA
        graphics::ScopedBind bound(aTexture);

        // Handle alignment
        Guard scopedAlignemnt = graphics::detail::scopeUnpackAlignment(aInput.alignment);

        gl.TexSubImage3D(aTexture.mTarget, aMipmapLevelId,
                         aTextureOffset.x(), aTextureOffset.y(), aTextureOffset.z(),
                         aInput.resolution.width(), aInput.resolution.height(), 1,
                         aInput.format, aInput.type,
                         aRawData);
    }


} // namespace proto


} // namespace ad::renderer