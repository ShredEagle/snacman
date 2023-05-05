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

// @brief Load a buffer range, controlled by byte offset and length.
std::vector<std::byte> 
loadBufferData(arte::Const_Owned<arte::gltf::Buffer> aBuffer,
               std::size_t aByteOffset,
               std::size_t aByteLength);


/// @brief Load the complete buffer view data, applying the view's offset & size to underlying buffer.
std::vector<std::byte> 
loadBufferViewData(arte::Const_Owned<arte::gltf::BufferView> aBufferView);

// TODO The logic to apply sparse accessor data is probably correct (dates back from gltf-viewer).
// Yet I think this interface will lead to missusing it in client code.
// Because this leads to loading a whole buffer view, but only applying one accessor sparse data if present.
// Disabled for the moment.
///// \brief Unified interface to handle both sparse and non-sparse accessors.
///// \attention Returns the complete underlying buffer view, accessor's offset and size are not applied!
//std::vector<std::byte> 
//loadWholeBufferViewData(arte::Const_Owned<arte::gltf::Accessor> aAccessor,
//                        arte::Const_Owned<arte::gltf::BufferView> aBufferView);

template <class T_pixel>
arte::Image<T_pixel> loadImageData(arte::Const_Owned<arte::gltf::Image> aImage);

// TODO Ad 2022/03/15 Implement a way to only load exactly the bytes of an accessor
// This could notably allow optimization when a contiguous accessor is used as-is.

// Implementation detail of loadTypedData()
// Makes a few checks and return a complete buffer view as array of bytes.
std::vector<std::byte> loadContiguousData_impl(arte::Const_Owned<arte::gltf::Accessor> aAccessor);

/// @brief Load the data for an accessor as a vector of T_value.
/// Only the accessor range is covered (because in general, a buffer view contains heterogenous types).
template <class T_value>
std::vector<T_value> loadTypedData(arte::Const_Owned<arte::gltf::Accessor> aAccessor)
{
    // TODO Ad 2023/05/03 This can probably be optimized to work without a copy
    // by only loading the accessor bytes (in case of no-stride).
    std::vector<std::byte> completeBuffer = loadContiguousData_impl(aAccessor);

    std::vector<T_value> result;
    result.reserve(aAccessor->count);
    T_value * first = reinterpret_cast<T_value *>(completeBuffer.data() + aAccessor->byteOffset);
    std::copy(first,
              first + aAccessor->count,
              std::back_inserter(result));
    return result;
}


} // namespace snac
} // namespace ad
