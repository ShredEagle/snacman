#include "RoundMonitor.h"
#include "snacman/simulations/snacgame/SceneGraph.h"
#include "snacman/simulations/snacgame/system/SceneGraphResolver.h"

#include "../component/PlayerGameData.h"
#include "../component/LevelData.h"
#include "../component/PlayerRoundData.h"
#include "../component/SceneNode.h"
#include "../component/PlayerSlot.h"
#include "../component/GlobalPose.h"

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

char RoundMonitor::updateRoundScore()
{
    std::pair<std::vector<component::PlayerGameData *>, int /*score*/>
        winners;
    winners.second = -1;
    char leaderBitMask = 0;
    mPlayers.each([&winners, &leaderBitMask](EntHandle aHandle,
                             component::PlayerRoundData & aRoundData) {

        component::PlayerGameData & gameData = snac::getComponent<component::PlayerGameData>(aRoundData.mSlot);
        if (winners.second < aRoundData.mRoundScore)
        {
            
            winners.first.clear();
            winners.first.push_back(&gameData);
            winners.second = aRoundData.mRoundScore;
            leaderBitMask = (char)(1 << aRoundData.mSlot.get()->get<component::PlayerSlot>().mSlotIndex);
        }
        else if (winners.second == aRoundData.mRoundScore)
        {
            winners.first.push_back(&gameData);
            leaderBitMask |= (char)(1 << aRoundData.mSlot.get()->get<component::PlayerSlot>().mSlotIndex);
        }
    });

    for (component::PlayerGameData * gameData : winners.first)
    {
        ++(gameData->mRoundsWon);
    }

    return leaderBitMask;
}
} // namespace system
} // namespace snacgame
} // namespace ad
