#pragma once


#include <arte/Image.h>
#include <arte/gltf/Gltf.h>

#include <renderer/AttributeDimension.h>


namespace ad {
namespace snac {

// TODO would benefit from a subnamespace, but gltf might be confusing with arte::gltf

//
// Helpers
//
/// \brief Returns the buffer view associated to the accessor, or throw if there is none.
arte::Const_Owned<arte::gltf::BufferView>
checkedBufferView(arte::Const_Owned<arte::gltf::Accessor> aAccessor);

arte::Const_Owned<arte::gltf::Image>
checkImage(arte::Const_Owned<arte::gltf::Texture> aTexture);

graphics::AttributeDimension getAccessorDimension(arte::gltf::Accessor::ElementType aElementType);

//
// Loaders
//
std::vector<std::byte> 
loadBufferData(arte::Const_Owned<arte::gltf::Buffer> aBuffer,
               std::size_t aByteOffset,
               std::size_t aByteLength);

/// \brief Unified interface to handle both sparse and non-sparse accessors.
/// \attention Returns the complete underlying buffer, offset and size are not applied!
std::vector<std::byte> 
loadBufferData(arte::Const_Owned<arte::gltf::Accessor> aAccessor,
               arte::Const_Owned<arte::gltf::BufferView> aBufferView);

template <class T_pixel>
arte::Image<T_pixel> loadImageData(arte::Const_Owned<arte::gltf::Image> aImage);

// TODO Ad 2022/03/15 Implement a way to only load exactly the bytes of an accessor
// This could notably allow optimization when a contiguous accessor is used as-is.

// Implementation detail of loadTypedData()
std::vector<std::byte> loadContiguousData_impl(arte::Const_Owned<arte::gltf::Accessor> aAccessor);

template <class T_value>
std::vector<T_value> loadTypedData(arte::Const_Owned<arte::gltf::Accessor> aAccessor)
{
    // TODO Ad 2023/05/03 This can probably be optimized to work without a copy
    // by only loading the accessor bytes (in case of no-stride).
    std::vector<std::byte> completeBuffer = loadContiguousData_impl(aAccessor);

    std::vector<T_value> result;
    result.reserve(aAccessor->count);
    T_value * first = reinterpret_cast<T_value *>(completeBuffer.data());
    std::copy(first, first + aAccessor->count, std::back_inserter(result));
    return result;
}


} // namespace snac
} // namespace ad
