#include "AdvanceAnimations.h"

#include "../GameContext.h"
#include "../component/PlayerSlot.h"
#include "../component/PlayerGameData.h"
#include "../component/RigAnimation.h"

#include <snacman/Timing.h>
#include <snacman/Logging.h>
#include <snacman/Profiling.h>
#include <snac-renderer-V1/Camera.h>


namespace ad {
namespace snacgame {
namespace system {
AdvanceAnimations::AdvanceAnimations(GameContext & aGameContext) :
    mAnimations{aGameContext.mWorld}
{}


void AdvanceAnimations::update(const snac::Time & aSimulationTime)
{
    TIME_RECURRING_CLASSFUNC(Main);

    mAnimations.each([&](component::RigAnimation & aRigAnimation)
    {
        // TODO Ad 2024/03/27: It should be asserted that the resulting value has enough precision as a float.
        aRigAnimation.mParameterValue = (float)aRigAnimation.getParameter(aSimulationTime.mTimepoint);
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
