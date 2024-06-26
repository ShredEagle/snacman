#pragma once

#include "Reflexion.h"

struct CompPart
{
    std::string a;
};

namespace component {
struct CompA
{
    int a;
    bool b;
    CompPart c;
};

SNAC_SERIAL_REGISTER(CompA)
};
