#pragma once

#include "entity/Entity.h"

#include <istream>
#include <ostream>

template <class T_ostreamable>
concept Ostreamable = requires(T_ostreamable v, std::ostream a) {
    {
        a << v
    };
};

template <class T_ostreamable>
concept Istreamable = requires(T_ostreamable v, std::istream a) {
    {
        a >> v
    };
};

struct SnacArchiveOut
{
    SnacArchiveOut(std::ostream & aStream) : mBuffer(aStream.rdbuf()) {}

    std::ostream mBuffer;

    template <Ostreamable T_ostreamable>
    SnacArchiveOut & operator&(T_ostreamable && aData)
    {
        mBuffer << aData;
        return *this;
    }
};

struct SnacArchiveIn
{
    std::istream mBuffer;

    template <Ostreamable T_ostreamable>
    SnacArchiveIn & operator&(T_ostreamable && data)
    {
        mBuffer >> data;
        return *this;
    }
};

template <class T_ostreamable>
concept Archivable = requires(T_ostreamable v, SnacArchiveOut a) {
    {
        a & v
    };
} && requires(T_ostreamable v, SnacArchiveIn a) {
    {
        a & v
    };
};
