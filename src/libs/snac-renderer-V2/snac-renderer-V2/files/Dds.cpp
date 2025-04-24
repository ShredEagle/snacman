#include "Dds.h"

#include "../Logging.h"

#include <cassert>


namespace ad::renderer::dds {

namespace {


    template <class T_destination> 
    void readBytes(std::istream & aIn, T_destination & aDestination)
    {
        aIn.read(reinterpret_cast<char *>(&aDestination), sizeof(T_destination));
    }


    template <class T_destination> 
    T_destination readBytes(std::istream & aIn)
    {
        T_destination destination;
        readBytes(aIn, destination);
        return destination;
    }


} // unnamed namespace


enum DWCAPS_enum : Dword_t
{
    DDSCAPS_COMPLEX = 0x8,
    DDSCAPS_MIPMAP = 0x400000,
    DDSCAPS_TEXTURE = 0x1000,
};


Header readHeader(std::istream & aDdsStream)
{
    // see: https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide#dds-file-layout
    std::uint32_t dwMagic;
    readBytes(aDdsStream, dwMagic);
    assert(dwMagic == 0x20534444); // "DDS ", little endian

    Header result;
    readBytes(aDdsStream, result.h);
    // Sanity checks
    assert((result.h.dwFlags & DDS_HEADER_FLAGS_TEXTURE) == DDS_HEADER_FLAGS_TEXTURE);
    assert((result.h.dwCaps & DDSCAPS_TEXTURE) == DDSCAPS_TEXTURE);
            
    if(((result.h.ddspf.dwFlags & DDPF_FOURCC) != 0)
        && result.h.ddspf.dwFourCC == 0x30315844) // "DX10", little endian
    {
        result.h_dxt10 = readBytes<DDS_HEADER_DXT10>(aDdsStream);
    }

    return result;
}


math::Size<2, GLint> getDimensions(const Header & aHeader)
{
    return {(GLint)aHeader.h.dwWidth, (GLint)aHeader.h.dwHeight};
}


// see: https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format
enum DXGI_FORMAT_enum : DXGI_FORMAT
{
  DXGI_FORMAT_UNKNOWN = 0,
  DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
  DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
  DXGI_FORMAT_R32G32B32A32_UINT = 3,
  DXGI_FORMAT_R32G32B32A32_SINT = 4,
  DXGI_FORMAT_R32G32B32_TYPELESS = 5,
  DXGI_FORMAT_R32G32B32_FLOAT = 6,
  DXGI_FORMAT_R32G32B32_UINT = 7,
  DXGI_FORMAT_R32G32B32_SINT = 8,
  DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
  DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
  DXGI_FORMAT_R16G16B16A16_UNORM = 11,
  DXGI_FORMAT_R16G16B16A16_UINT = 12,
  DXGI_FORMAT_R16G16B16A16_SNORM = 13,
  DXGI_FORMAT_R16G16B16A16_SINT = 14,
  DXGI_FORMAT_R32G32_TYPELESS = 15,
  DXGI_FORMAT_R32G32_FLOAT = 16,
  DXGI_FORMAT_R32G32_UINT = 17,
  DXGI_FORMAT_R32G32_SINT = 18,
  DXGI_FORMAT_R32G8X24_TYPELESS = 19,
  DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
  DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
  DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
  DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
  DXGI_FORMAT_R10G10B10A2_UNORM = 24,
  DXGI_FORMAT_R10G10B10A2_UINT = 25,
  DXGI_FORMAT_R11G11B10_FLOAT = 26,
  DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
  DXGI_FORMAT_R8G8B8A8_UNORM = 28,
  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
  DXGI_FORMAT_R8G8B8A8_UINT = 30,
  DXGI_FORMAT_R8G8B8A8_SNORM = 31,
  DXGI_FORMAT_R8G8B8A8_SINT = 32,
  DXGI_FORMAT_R16G16_TYPELESS = 33,
  DXGI_FORMAT_R16G16_FLOAT = 34,
  DXGI_FORMAT_R16G16_UNORM = 35,
  DXGI_FORMAT_R16G16_UINT = 36,
  DXGI_FORMAT_R16G16_SNORM = 37,
  DXGI_FORMAT_R16G16_SINT = 38,
  DXGI_FORMAT_R32_TYPELESS = 39,
  DXGI_FORMAT_D32_FLOAT = 40,
  DXGI_FORMAT_R32_FLOAT = 41,
  DXGI_FORMAT_R32_UINT = 42,
  DXGI_FORMAT_R32_SINT = 43,
  DXGI_FORMAT_R24G8_TYPELESS = 44,
  DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
  DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
  DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
  DXGI_FORMAT_R8G8_TYPELESS = 48,
  DXGI_FORMAT_R8G8_UNORM = 49,
  DXGI_FORMAT_R8G8_UINT = 50,
  DXGI_FORMAT_R8G8_SNORM = 51,
  DXGI_FORMAT_R8G8_SINT = 52,
  DXGI_FORMAT_R16_TYPELESS = 53,
  DXGI_FORMAT_R16_FLOAT = 54,
  DXGI_FORMAT_D16_UNORM = 55,
  DXGI_FORMAT_R16_UNORM = 56,
  DXGI_FORMAT_R16_UINT = 57,
  DXGI_FORMAT_R16_SNORM = 58,
  DXGI_FORMAT_R16_SINT = 59,
  DXGI_FORMAT_R8_TYPELESS = 60,
  DXGI_FORMAT_R8_UNORM = 61,
  DXGI_FORMAT_R8_UINT = 62,
  DXGI_FORMAT_R8_SNORM = 63,
  DXGI_FORMAT_R8_SINT = 64,
  DXGI_FORMAT_A8_UNORM = 65,
  DXGI_FORMAT_R1_UNORM = 66,
  DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
  DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
  DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
  DXGI_FORMAT_BC1_TYPELESS = 70,
  DXGI_FORMAT_BC1_UNORM = 71,
  DXGI_FORMAT_BC1_UNORM_SRGB = 72,
  DXGI_FORMAT_BC2_TYPELESS = 73,
  DXGI_FORMAT_BC2_UNORM = 74,
  DXGI_FORMAT_BC2_UNORM_SRGB = 75,
  DXGI_FORMAT_BC3_TYPELESS = 76,
  DXGI_FORMAT_BC3_UNORM = 77,
  DXGI_FORMAT_BC3_UNORM_SRGB = 78,
  DXGI_FORMAT_BC4_TYPELESS = 79,
  DXGI_FORMAT_BC4_UNORM = 80,
  DXGI_FORMAT_BC4_SNORM = 81,
  DXGI_FORMAT_BC5_TYPELESS = 82,
  DXGI_FORMAT_BC5_UNORM = 83,
  DXGI_FORMAT_BC5_SNORM = 84,
  DXGI_FORMAT_B5G6R5_UNORM = 85,
  DXGI_FORMAT_B5G5R5A1_UNORM = 86,
  DXGI_FORMAT_B8G8R8A8_UNORM = 87,
  DXGI_FORMAT_B8G8R8X8_UNORM = 88,
  DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
  DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
  DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
  DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
  DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
  DXGI_FORMAT_BC6H_TYPELESS = 94,
  DXGI_FORMAT_BC6H_UF16 = 95,
  DXGI_FORMAT_BC6H_SF16 = 96,
  DXGI_FORMAT_BC7_TYPELESS = 97,
  DXGI_FORMAT_BC7_UNORM = 98,
  DXGI_FORMAT_BC7_UNORM_SRGB = 99,
  DXGI_FORMAT_AYUV = 100,
  DXGI_FORMAT_Y410 = 101,
  DXGI_FORMAT_Y416 = 102,
  DXGI_FORMAT_NV12 = 103,
  DXGI_FORMAT_P010 = 104,
  DXGI_FORMAT_P016 = 105,
  DXGI_FORMAT_420_OPAQUE = 106,
  DXGI_FORMAT_YUY2 = 107,
  DXGI_FORMAT_Y210 = 108,
  DXGI_FORMAT_Y216 = 109,
  DXGI_FORMAT_NV11 = 110,
  DXGI_FORMAT_AI44 = 111,
  DXGI_FORMAT_IA44 = 112,
  DXGI_FORMAT_P8 = 113,
  DXGI_FORMAT_A8P8 = 114,
  DXGI_FORMAT_B4G4R4A4_UNORM = 115,
  DXGI_FORMAT_P208 = 130,
  DXGI_FORMAT_V208 = 131,
  DXGI_FORMAT_V408 = 132,
  DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE,
  DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE,
  DXGI_FORMAT_FORCE_UINT = 0xffffffff
};


GLenum getCompressedFormat(const Header & aHeader)
{
    if(aHeader.h_dxt10)
    {
        const DDS_HEADER_DXT10 & dxt10 = *aHeader.h_dxt10;
        switch(dxt10.dxgiFormat)
        {
            default:
                // TODO Ad 2024/07/24: Extend to support a reasonable set of formats.
                SELOG(error)("DXGI format {} is not supported at the moment", dxt10.dxgiFormat);
                throw std::domain_error("The texture format in this DDS is not supported at the moment.");
            case DXGI_FORMAT_BC5_UNORM:
                return GL_COMPRESSED_RG_RGTC2;
            case DXGI_FORMAT_BC5_SNORM:
                return GL_COMPRESSED_SIGNED_RG_RGTC2;
            case DXGI_FORMAT_BC6H_UF16:
                return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
            case DXGI_FORMAT_BC6H_SF16:
                return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
            case DXGI_FORMAT_BC7_UNORM:
                return GL_COMPRESSED_RGBA_BPTC_UNORM;
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
        }
    }
    throw std::invalid_argument("This DDS does not contain an extended DXT10 header.");
}


GLsizei getCompressedByteSize(const Header & aHeader)
{
    if ((aHeader.h.dwFlags & DDSD_LINEARSIZE) != 0)
    {
        return aHeader.h.dwPitchOrLinearSize;
    }

    throw std::domain_error("The texture in this DDS does not provide its linear size.");
}


enum DWCAPS2_enum : Dword_t
{
    DDSCAPS2_CUBEMAP = 0x200,
    DDSCAPS2_CUBEMAP_POSITIVEX = 0x400,
    DDSCAPS2_CUBEMAP_NEGATIVEX = 0x800,
    DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000,
    DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000,
    DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000,
    DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000,
    DDSCAPS2_VOLUME = 0x200000,
};


Dword_t DDS_CUBEMAP_POSITIVEX = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX;
// etc.
Dword_t DDS_CUBEMAP_ALLFACES = 
    DDSCAPS2_CUBEMAP
    | DDSCAPS2_CUBEMAP_POSITIVEX
    | DDSCAPS2_CUBEMAP_NEGATIVEX
    | DDSCAPS2_CUBEMAP_POSITIVEY
    | DDSCAPS2_CUBEMAP_NEGATIVEY
    | DDSCAPS2_CUBEMAP_POSITIVEZ
    | DDSCAPS2_CUBEMAP_NEGATIVEZ
    ;


GLenum getTextureTarget(const Header & aHeader)
{
    if((aHeader.h.dwCaps2 & DDSCAPS2_CUBEMAP) == DDSCAPS2_CUBEMAP)
    {
        assert((aHeader.h.dwCaps & DDSCAPS_COMPLEX) == DDSCAPS_COMPLEX);
        if(aHeader.h.dwCaps2 == DDS_CUBEMAP_ALLFACES)
        {
            return GL_TEXTURE_CUBE_MAP;
        }
        else
        {
            SELOG(critical)("Partial cube-maps are not supported.");
            throw std::invalid_argument{"Partial cube-maps are not supported."};
        }
    }
    else if(aHeader.h_dxt10)
    {
        switch(aHeader.h_dxt10->resourceDimension)
        {
            case DDS_DIMENSION_TEXTURE1D:
                return GL_TEXTURE_1D;
            case DDS_DIMENSION_TEXTURE2D:
                return GL_TEXTURE_2D;
            case DDS_DIMENSION_TEXTURE3D:
                return GL_TEXTURE_3D;
        }
    }

    // TODO complete when this exception is thrown
    throw std::domain_error{"Could not derive a target from the DDS header."};
}

} // namespace ad::renderer::dds