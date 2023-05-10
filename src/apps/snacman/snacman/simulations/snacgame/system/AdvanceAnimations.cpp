#include "AdvanceAnimations.h"

#include <snacman/Logging.h>
#include <snacman/Profiling.h>


namespace ad {
namespace snacgame {
namespace system {

void AdvanceAnimations::update()
{
    TIME_RECURRING_CLASSFUNC(Main);

    mAnimations.each([&](component::RigAnimation & aRigAnimation)
    {
        aRigAnimation.mParameterValue = aRigAnimation.getParameter(snac::Clock::now());
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
