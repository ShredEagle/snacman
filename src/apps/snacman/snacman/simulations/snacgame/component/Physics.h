
#include "reflexion/NameValuePair.h"
#include <snacman/serialization/Serial.h>
namespace ad {
namespace snacgame {
namespace component {

constexpr float gDefaultGravity = -9.81f * 3.f;

struct Gravity
{
    float mFloorHeight{0.f};
    float mGravityAccel = gDefaultGravity;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mFloorHeight));
        aWitness.witness(NVP(mGravityAccel));
    }
};

REFLEXION_REGISTER(Gravity)

}
}
}
