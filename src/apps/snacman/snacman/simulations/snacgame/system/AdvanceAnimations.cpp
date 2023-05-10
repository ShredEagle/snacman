#include "AdvanceAnimations.h"

#include <snacman/Logging.h>
#include <snacman/Profiling.h>


namespace ad {
namespace snacgame {
namespace system {

void AdvanceAnimations::update(const snac::Time & aTime)
{
    TIME_RECURRING_CLASSFUNC(Main);

    mAnimations.each([&](component::RigAnimation & aRigAnimation)
    {
        aRigAnimation.mParameterValue = aRigAnimation.getParameter(aTime.mSimulationTimepoint);
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
