#include "Mesh.h"


namespace ad {
namespace snac {


std::ostream & operator<<(std::ostream & aOut, const Model & aModel)
{
    if(aModel.mName.empty())
    {
        return aOut << "unnamed mesh (at " << &aModel << ")";
    }
    else
    {
        return aOut << aModel.mName << "(at " << &aModel << ")";
    }
}


} // namespace snac
} // namespace ad