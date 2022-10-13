#include "Logging.h"


using namespace ad;


int main(int argc, char * argv[])
{
    snac::detail::initializeLogging();

    SELOG(info)("I'm a snac man.");
    return 0;
}
