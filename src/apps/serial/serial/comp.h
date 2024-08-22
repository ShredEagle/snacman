#pragma once

#include "serialization/Serialization.h"
#include "reflexion/Reflexion.h"

#include <reflexion/Concept.h>
#include <entity/Entity.h>

namespace component {

struct CompPart
{
    std::string a;

    template<class T_witness>
    void describeTo(T_witness & aWitness)
    {
        aWitness & NVP(a);
    }
};

REFLEXION_REGISTER(CompPart)

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


    template<class T_witness>
    void describeTo(T_witness & aWitness)
    {
        aWitness & NVP(a);
        aWitness & NVP(b);
        aWitness & NVP(c);
        aWitness & NVP(array);
        aWitness & NVP(other);
        aWitness & NVP_FN(x);
        aWitness & NVP_FN(y);
        aWitness & NVP_FN(z);
    }
};

REFLEXION_REGISTER(CompA)
};
