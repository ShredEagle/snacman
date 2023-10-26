#include <istream>
#include <ostream>

struct SnacArchiveOut
{
    std::ostream mBuffer;

    // We will need some structure to map handles
};

struct SnacArchiveIn
{
    std::istream mBuffer;
};
