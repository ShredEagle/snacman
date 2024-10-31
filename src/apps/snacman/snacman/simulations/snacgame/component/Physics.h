
#include "reflexion/NameValuePair.h"
#include <snacman/serialization/Serial.h>
namespace ad {
namespace snacgame {
namespace component {

constexpr float gDefaultGravity = -9.81f * 2.f;

struct Gravity
{
    float mFloorHeight;
    float mGravityAccel = gDefaultGravity;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mFloorHeight));
    }
};

REFLEXION_REGISTER(Gravity)

}
}
}
