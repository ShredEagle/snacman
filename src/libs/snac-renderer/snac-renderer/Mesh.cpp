#include "Mesh.h"


namespace ad {
namespace snac {


std::ostream & operator<<(std::ostream & aOut, const Mesh & aMesh)
{
    if(aMesh.mName.empty())
    {
        return aOut << "unnamed mesh (at " << &aMesh << ")";
    }
    else
    {
        return aOut << aMesh.mName << "(at " << &aMesh << ")";
    }
}


} // namespace snac
} // namespace ad