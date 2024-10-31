#include "math/Vector.h"
#include "reflexion/NameValuePair.h"
#include "snac-reflexion/Reflexion.h"
namespace ad {
namespace component {

struct MassSpringDamperScreenSpace
{
    math::Position<2, float> mAnchor;
    float mSpringStrength;
    float mDamping;
    float mMass;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mAnchor));
        aWitness.witness(NVP(mSpringStrength));
        aWitness.witness(NVP(mDamping));
        aWitness.witness(NVP(mMass));
    }
};

REFLEXION_REGISTER(MassSpringDamperScreenSpace)
    
}
}
