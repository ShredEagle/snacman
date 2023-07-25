#pragma once


#include <math/Matrix.h>

#include <fstream>
#include <span>


//
// IMPORTANT: keep it header only for the moment, it is dirty included from sibling applications.
//

namespace ad::renderer {


struct BinaryOutArchive
{
    template <class T> requires std::is_arithmetic_v<T>
    BinaryOutArchive & write(T aValue)
    {
        mOut.write((const char *)&aValue, sizeof(T));
        return *this;
    }

    template <class T>
    BinaryOutArchive & write(std::span<T> aData)
    {
        mOut.write((const char *)aData.data(), aData.size_bytes());
        return *this;
    }

    template <int N_rows, int N_cols, class T_number>
    BinaryOutArchive & write(const math::Matrix<N_rows, N_cols, T_number> & aMatrix)
    {
        return write(std::span{aMatrix});
    }


    std::ofstream mOut;
};


struct BinaryInArchive
{
    template <class T> requires std::is_arithmetic_v<T>
    BinaryInArchive & read(T & aValue)
    {
        mIn.read((char *)&aValue, sizeof(T));
        return *this;
    }

    std::unique_ptr<char[]> readBytes(std::size_t aCount)
    {
        std::unique_ptr<char[]> buffer{new char[aCount]};
        mIn.read(buffer.get(), aCount);
        return buffer;
    }

    template <class T>
    BinaryInArchive & read(std::span<T> aData)
    {
        mIn.read((char *)aData.data(), aData.size_bytes());
        return *this;
    }

    template <int N_rows, int N_cols, class T_number>
    BinaryInArchive & read(math::Matrix<N_rows, N_cols, T_number> & aMatrix)
    {
        return read(std::span{aMatrix});
    }

    std::ifstream mIn;
};


} // namespace ad::renderer