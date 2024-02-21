#pragma once

#include "Reflexion.h"
#include "entity/Entity.h"
#include "Serialization.h"

namespace component {

struct CompPart
{
    std::string a;

    template <SnacSerial T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(a);
    }
};

SNAC_SERIAL_REGISTER(CompPart)

struct CompA
{
    int a;
    bool b;
    CompPart c;
    ad::ent::Handle<ad::ent::Entity> other;
    std::array<int, 3> array;

    int & x() {
        return array[0];
    }

    int & y() {
        return array[1];
    }

    int & z() {
        return array[2];
    }


    template <SnacSerial T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(a);
        archive & SERIAL_PARAM(b);
        archive & SERIAL_PARAM(c);
        archive & SERIAL_PARAM(other);
        archive & SERIAL_FN_PARAM(x);
        archive & SERIAL_FN_PARAM(y);
        archive & SERIAL_FN_PARAM(z);
    }
};

SNAC_SERIAL_REGISTER(CompA)
};
