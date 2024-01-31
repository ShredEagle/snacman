#include "LoadBuffer.h"

#include "../Logging.h"

#include <handy/Base64.h>
#include <handy/Url.h>

#include <span>

#include <strstream>
#include <fstream>

#include <renderer/AttributeDimension.h>
//#include <renderer/GL_Loader.h>
#include <renderer/MappedGL.h>



#define SELOG(level) SELOG_LG(::ad::snac::gGltfLogger, level)


namespace ad {
namespace snac {


namespace {

    // TODO Should be moved to a more general library.
    // Reading a stream content is a recurring need.
    std::vector<std::byte> loadInputStream(std::istream & aInput, std::size_t aByteLength, const std::string & aStreamId)
    {
        constexpr std::size_t gChunkSize = 128 * 1024;

        std::vector<std::byte> result(aByteLength);
        std::size_t remainingBytes = aByteLength;

        char * destination = reinterpret_cast<char *>(result.data());
        while (remainingBytes)
        {
            std::size_t readSize = std::min(remainingBytes, gChunkSize);
            if (!aInput.read(destination, readSize).good())
            {
                throw std::runtime_error{"Problem reading '" + aStreamId + "': "
                    // If stream is not good(), yet converts to 'true', it means only eofbit is set.
                    + (aInput ? "stream truncated." : "read error.")};
            }
            destination += readSize;
            remainingBytes -= readSize;
        }
        return result;
    }


    using ElementType = arte::gltf::Accessor::ElementType;

    const std::map<ElementType, graphics::AttributeDimension> gElementTypeToLayout{
        {ElementType::Scalar, {1}},
        {ElementType::Vec2,   {2}},
        {ElementType::Vec3,   {3}},
        {ElementType::Vec4,   {4}},
        {ElementType::Mat2,   {2, 2}},
        {ElementType::Mat3,   {3, 3}},
        {ElementType::Mat4,   {4, 4}},
    };


    std::size_t getElementByteSize(arte::Const_Owned<arte::gltf::Accessor> aAccessor)
    {
        return getAccessorDimension(aAccessor->type).countComponents()
            * graphics::getByteSize(aAccessor->componentType);
    }

} // unnamed namespace


graphics::AttributeDimension getAccessorDimension(arte::gltf::Accessor::ElementType aElementType)
{
    return gElementTypeToLayout.at(aElementType);
}

arte::Const_Owned<arte::gltf::BufferView>
checkedBufferView(arte::Const_Owned<arte::gltf::Accessor> aAccessor)
{
    if (!aAccessor->bufferView)
    {
        SELOG(critical)
             ("Unsupported: Accessor #{} does not have a buffer view associated.", fmt::streamed(aAccessor.id()));
        throw std::logic_error{"Accessor was expected to have a buffer view."};
    }
    return aAccessor.get(&arte::gltf::Accessor::bufferView);
}


arte::Const_Owned<arte::gltf::Image>
checkImage(arte::Const_Owned<arte::gltf::Texture> aTexture)
{
    if (!aTexture->source)
    {
        // Note: By the spec, when source is not provided, an extension or other mechanism
        // **should** provide an alternate texture source.
        SELOG(critical)
             ("Unsupported: Texture #{} does not have a source associated.", fmt::streamed(aTexture.id()));
        throw std::logic_error{"Texute was expected to have a source image."};
    }
    return aTexture.get(&arte::gltf::Texture::source);
}


std::vector<std::byte> loadDataUri(arte::gltf::Uri aUri)
{
    constexpr const char * base64Prefix{"base64,"};

    std::string_view encoded{aUri.string};
    encoded.remove_prefix(encoded.find(base64Prefix) + std::strlen(base64Prefix));
    return handy::base64::decode(encoded);
}


std::vector<std::byte> loadBufferData(arte::Const_Owned<arte::gltf::Buffer> aBuffer,
                                      std::size_t aByteOffset,
                                      std::size_t aByteLength)
{
    if (!aBuffer->uri)
    {
        SELOG(critical)
             ("Buffer #{} does not have target defined.", fmt::streamed(aBuffer.id()));
        throw std::logic_error{"Buffer was expected to have an Uri."};
    }

    if(aBuffer->byteLength < aByteOffset + aByteLength)
    {
        SELOG(critical)
            ("Attempt to read range [{}, {}] from buffer #{} of length {} bytes.",
            aByteOffset, aByteLength, fmt::streamed(aBuffer.id()), aBuffer->byteLength);
        throw std::out_of_range{"Loading buffer past the end."};
    }

    auto uri = *aBuffer->uri;
    switch(uri.type)
    {
    case arte::gltf::Uri::Type::Data:
    {
        // TODO implement handling subranges on data Uri.
        if(aByteOffset != 0 || aByteLength != aBuffer->byteLength)
        {

            SELOG(critical)
                 ("Loading a subset of Uri data buffer #{} is not implemented yet.", fmt::streamed(aBuffer.id()));
        /*     throw std::invalid_argument{"Loading a subset of Uri data buffer."}; */
        }
        SELOG(trace)("Buffer #{} data is read from a data URI.", fmt::streamed(aBuffer.id()));
        std::vector<std::byte> data = loadDataUri(uri);
        
        return std::vector<std::byte>(data.begin() + aByteOffset, data.begin() + aByteOffset + aByteLength);
    }
    case arte::gltf::Uri::Type::File:
    {
        SELOG(trace)("Buffer #{} data is read from file {}.", fmt::streamed(aBuffer.id()), uri.string);
        std::ifstream fileStream{
            handy::decodeUrl(aBuffer.getFilePath(&arte::gltf::Buffer::uri).string()),
            std::ios_base::in | std::ios_base::binary
        };
        fileStream.seekg(aByteOffset); // Advance by the offset
        return loadInputStream(
            fileStream,
            aByteLength,
            uri.string);
    }
    default:
        throw std::logic_error{"Invalid uri type."};
    }
}


std::vector<std::byte> loadBufferViewData(arte::Const_Owned<arte::gltf::BufferView> aBufferView)
{
    return loadBufferData(
        aBufferView.get(&arte::gltf::BufferView::buffer),
        aBufferView->byteOffset,
        aBufferView->byteLength);
}


template <class T_component>
std::vector<GLuint> copyIndices(const std::byte * aFirst, std::size_t aCount)
{
    const T_component * first = reinterpret_cast<const T_component *>(aFirst);
    std::vector<GLuint> result;
    std::copy(first, first + aCount, std::back_inserter(result));
    return result;
}

/// \brief Converts all potential indice types to the largest representation.
std::vector<GLuint>
loadIndices(arte::Const_Owned<arte::gltf::accessor::Indices> aIndices, std::size_t aCount)
{
    auto bufferView = aIndices.get(&arte::gltf::accessor::Indices::bufferView);
    std::vector<std::byte> buffer = loadBufferViewData(bufferView);

    std::byte * first = buffer.data() + bufferView->byteOffset + aIndices->byteOffset;
    switch(aIndices->componentType)
    {
    case GL_UNSIGNED_BYTE:
        return copyIndices<GLubyte>(first, aCount);
    case GL_UNSIGNED_SHORT:
        return copyIndices<GLushort>(first, aCount);
    case GL_UNSIGNED_INT:
        return copyIndices<GLuint>(first, aCount);
    }

    throw std::logic_error{std::string{"In "} + __func__
        + ", unhandled component type: " + std::to_string(aIndices->componentType)};
}


// Keep, for the sparse logic
//std::vector<std::byte>
//loadWholeBufferViewData(arte::Const_Owned<arte::gltf::Accessor> aAccessor,
//                        arte::Const_Owned<arte::gltf::BufferView> aBufferView)
//{
//    std::vector<std::byte> bufferData = loadBufferData(aBufferView);
//
//    if(aAccessor->sparse)
//    {
//        auto sparse = aAccessor.get(&arte::gltf::Accessor::sparse);
//        std::vector<GLuint> indices =
//            loadIndices(sparse.get(&arte::gltf::accessor::Sparse::indices), sparse->count);
//
//        auto values = sparse.get(&arte::gltf::accessor::Sparse::values);
//        auto valuesBufferView = values.get(&arte::gltf::accessor::Values::bufferView);
//        std::vector<std::byte> differenceBuffer = loadBufferData(valuesBufferView);
//        const std::byte * difference =
//            differenceBuffer.data() + valuesBufferView->byteOffset + values->byteOffset;
//
//        std::byte * element =
//            bufferData.data() + aBufferView->byteOffset + aAccessor->byteOffset;
//        const std::size_t elementSize = getElementByteSize(aAccessor);
//
//        std::size_t iteration = 0;
//        for (auto modifiedIndex : indices)
//        {
//            std::copy(difference + iteration * elementSize,
//                      difference + (iteration + 1) * elementSize,
//                      element + modifiedIndex * elementSize);
//            ++iteration;
//        }
//    }
//
//    return bufferData;
//}


const std::map<arte::gltf::Image::MimeType, arte::ImageFormat> gMimeToFormat {
    {arte::gltf::Image::MimeType::ImageJpeg, arte::ImageFormat::Jpg},
    {arte::gltf::Image::MimeType::ImagePng, arte::ImageFormat::Png},
};



template <class T_pixel>
arte::Image<T_pixel>
loadImageFromBytes(std::span<std::byte> aBytes, arte::gltf::Image::MimeType aMime)
{
    using Image = arte::Image<T_pixel>;

    // TODO Ad 2022/03/23: Replace deprecated istrstream with a proposed alternative
    // see: https://en.cppreference.com/w/cpp/io/istrstream
    // see: https://stackoverflow.com/a/12646922/1027706
    return Image::Read(
        gMimeToFormat.at(aMime),
        std::istrstream(reinterpret_cast<const char *>(aBytes.data()), aBytes.size()),
        arte::ImageOrientation::Unchanged);
}


template <class T_pixel>
arte::Image<T_pixel> loadImageData(arte::Const_Owned<arte::gltf::Image> aImage)
{
    using Image = arte::Image<T_pixel>;

    if(const arte::gltf::Uri * uri = std::get_if<arte::gltf::Uri>(&aImage->dataSource))
    {
        switch(uri->type)
        {
        case arte::gltf::Uri::Type::Data:
        {
            SELOG(trace)("Image #{} data is read from a data URI.", fmt::streamed(aImage.id()));
            // TODO handle this situation by reading the magic numbers
            if (!aImage->mimeType)
            {
                SELOG(critical)
                     ("Unsupported: Image #{} has a data URI but no mime type.", fmt::streamed(aImage.id()));
                throw std::logic_error{"Image with data URI but no mime type."};
            }
            auto bytes = loadDataUri(*uri);
            return loadImageFromBytes<T_pixel>(bytes, *aImage->mimeType);
        }
        case arte::gltf::Uri::Type::File:
        {
            SELOG(trace)("Image #{} data is read from file '{}'.", fmt::streamed(aImage.id()), uri->string);
            return Image{handy::decodeUrl(aImage.getFilePath(*uri).string()), arte::ImageOrientation::Unchanged};
        }
        default:
            throw std::logic_error{"Invalid uri type."};
        }
    }
    else
    {
        SELOG(trace)("Image #{} data is read from a buffer view.", fmt::streamed(aImage.id()));
        auto bufferView =
            aImage.get(std::get<arte::gltf::Index<arte::gltf::BufferView>>(aImage->dataSource));

        if (!aImage->mimeType)
        {
            SELOG(critical)
                 ("Invalid file: Image #{} has a buffer view but no mime type.", fmt::streamed(aImage.id()));
            throw std::logic_error{"Image with buffer view but no mime type."};
        }

        auto bytes = loadBufferViewData(bufferView);
        //return loadImageFromBytes(std::span<std::byte>{bytes}, *aImage->mimeType);
        return loadImageFromBytes<T_pixel>(bytes, *aImage->mimeType);
    }
}


// Implementation detail of loadTypedData()
std::vector<std::byte> loadContiguousData_impl(arte::Const_Owned<arte::gltf::Accessor> aAccessor)
{
    // TODO Handle no buffer view (accessor initialized to zeros)
    auto bufferView = checkedBufferView(aAccessor);
    
    // Not implemented for the moment
    assert(!aAccessor->sparse);

    // TODO Handle stride (might not always be allowed though: e.g. in animation buffer views?)
    if(bufferView->byteStride)
    {
        if (*bufferView->byteStride == getElementByteSize(aAccessor))
        {
            SELOG(trace)
                 ("Buffer view #{} has an explicit stride of {}, which matches the element byte size.",
                  fmt::streamed(*aAccessor->bufferView), *bufferView->byteStride);
        }
        else
        {
            SELOG(critical)
                 ("Buffer view #{} has an non-default stride, not currently supported when loading typed data.",
                  fmt::streamed(*aAccessor->bufferView));
            throw std::logic_error{"Buffer view with stride when loading typed data."};
        }
    }

    return loadBufferViewData(bufferView);
}


//
// Explicit template instanciations
//

template arte::Image<math::sdr::Rgba> loadImageData(arte::Const_Owned<arte::gltf::Image> aImage);
template arte::Image<math::sdr::Rgb>  loadImageData(arte::Const_Owned<arte::gltf::Image> aImage);


} // namespace snac
} // namespace ad
