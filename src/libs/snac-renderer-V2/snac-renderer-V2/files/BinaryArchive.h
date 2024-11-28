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
    using Position_t = std::ofstream::pos_type;

    template <class T> requires std::is_arithmetic_v<T>
    BinaryOutArchive & write(T aValue)
    {
        mOut.write((const char *)&aValue, sizeof(T));
        return *this;
    }

    Position_t getPosition()
    {
        return mOut.tellp();
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

    template <int N_dimension, class T_number>
    BinaryOutArchive & write(const math::Size<N_dimension, T_number> aSize)
    {
        for(std::size_t idx = 0; idx != aSize.Dimension; ++idx)
        { 
            write(aSize[idx]);
        }
        return *this;
    }

    BinaryOutArchive & write(const std::string & aString)
    {
        write((unsigned int)aString.size());
        return write(std::span{aString});
    }

    template <class T_key, class T_value>
    BinaryOutArchive & write(const std::unordered_map<T_key, T_value> & aMap)
    {
        write((unsigned int)aMap.size());
        for(const auto & [key, value] : aMap)
        {
            write(key);
            write(value);
        }
        return *this;
    }

    std::ofstream mOut;
    std::filesystem::path mParentPath;
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

    BinaryInArchive & read(std::string & aString)
    {
        aString = readString();
        return *this;
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

    template <int N_dimension, class T_number>
    BinaryInArchive & read(math::Size<N_dimension, T_number> & aSize)
    {
        for(std::size_t idx = 0; idx != aSize.Dimension; ++idx)
        { 
            read(aSize[idx]);
        }
        return *this;
    }

    template <class T_key, class T_value>
    BinaryInArchive & read(std::unordered_map<T_key, T_value> & aMap)
    {
        unsigned int mapSize;
        read(mapSize);
        T_key key;
        T_value value;
        for(unsigned int i = 0; i != mapSize; ++i)
        {
            read(key);
            read(value);
            aMap.emplace(key, value);
        }
        return *this;
    }

    std::ifstream mIn;
    std::filesystem::path mParentPath;
};


template <class T_value>
T_value readAs(BinaryInArchive & aIn)
{
    T_value v;
    aIn.read(v);
    return v;
}


template <class T_element>
std::vector<T_element> readAsVector(BinaryInArchive & aIn,
                                    std::size_t aElementCount,
                                    T_element aDefaultValue = {})
{
    std::vector<T_element> result(aElementCount, aDefaultValue);
    aIn.read(std::span{result});
    return result;
}


template <class T_element>
void assignVector(std::vector<T_element> & aVector,
                  BinaryInArchive & aIn,
                  std::size_t aElementCount,
                  T_element aDefaultValue = {})
{
    aVector = readAsVector<T_element>(aIn, aElementCount, aDefaultValue);
}


} // namespace ad::renderer