#include "RoundMonitor.h"

#include "../component/PlayerGameData.h"
#include "../component/LevelData.h"
#include "../component/PlayerRoundData.h"
#include "../component/SceneNode.h"
#include "../component/PlayerSlot.h"

#include "../Entities.h"
#include "../GameContext.h"
#include "../typedef.h"

#include <snacman/EntityUtilities.h>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {

RoundMonitor::RoundMonitor(GameContext & aGameContext) :
    mGameContext{&aGameContext},
    mPlayers{mGameContext->mWorld},
    mPills{mGameContext->mWorld},
    mLevelEntities{mGameContext->mWorld}
{}

bool RoundMonitor::isRoundOver()
{
    return mPills.countMatches() == 0;
}

void RoundMonitor::updateRoundScore()
{
    std::pair<std::vector<component::PlayerGameData *>, int /*score*/>
        winners;
    winners.second = -1;
    mPlayers.each([&winners](EntHandle aHandle,
                             component::PlayerRoundData & aRoundData) {
        component::PlayerGameData & gameData = snac::getComponent<component::PlayerGameData>(aRoundData.mSlotHandle);
        if (winners.second < aRoundData.mRoundScore)
        {
            
            winners.first.clear();
            winners.first.push_back(&gameData);
            winners.second = aRoundData.mRoundScore;
        }
        else if (winners.second == aRoundData.mRoundScore)
        {
            winners.first.push_back(&gameData);
        }
    });

    for (auto & lifeCycle : winners.first)
    {
        ++(lifeCycle->mRoundsWon);
    }

}
} // namespace system
} // namespace snacgame
} // namespace ad
