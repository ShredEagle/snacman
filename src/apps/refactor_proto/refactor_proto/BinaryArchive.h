#pragma once


#include <math/Box.h>
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

    template <class T_derived, int N_rows, int N_cols, class T_number>
    BinaryOutArchive & write(const math::MatrixBase<T_derived, N_rows, N_cols, T_number> & aMatrix)
    {
        return write(std::span{aMatrix});
    }

    template <class T_number>
    BinaryOutArchive & write(const math::Box<T_number> & aBox)
    {
        write(aBox.mPosition);
        write(aBox.mDimension);
        return *this;
    }

    BinaryOutArchive & write(const std::string & aString)
    {
        write((unsigned int)aString.size());
        return write(std::span{aString});
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

    std::string readString()
    {
        unsigned int stringSize;
        read(stringSize);
        std::string buffer(stringSize, '\0');
        mIn.read(buffer.data(), stringSize);
        return buffer;
    }

    template <class T>
    BinaryInArchive & read(std::span<T> aData)
    {
        mIn.read((char *)aData.data(), aData.size_bytes());
        return *this;
    }

    template <class T_derived, int N_rows, int N_cols, class T_number>
    BinaryInArchive & read(math::MatrixBase<T_derived, N_rows, N_cols, T_number> & aMatrix)
    {
        return read(std::span{aMatrix});
    }

    template <class T_number>
    BinaryInArchive & read(math::Box<T_number> & aBox)
    {
        read(aBox.mPosition);
        read(aBox.mDimension);
        return *this;
    }

    std::ifstream mIn;
    std::filesystem::path mParentPath;
};


} // namespace ad::renderer