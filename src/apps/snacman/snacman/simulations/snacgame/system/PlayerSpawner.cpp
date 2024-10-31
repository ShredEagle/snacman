#include "PlayerSpawner.h"

#include "SceneGraphResolver.h"
#include "snacman/simulations/snacgame/GameParameters.h"
#include "snacman/simulations/snacgame/component/Speed.h"
#include "snacman/simulations/snacgame/component/Physics.h"

#include "../component/Controller.h"
#include "../component/GlobalPose.h"
#include "../component/PlayerGameData.h"
#include "../component/PlayerRoundData.h"
#include "../component/PlayerSlot.h"
#include "../component/SceneNode.h"
#include "../component/Spawner.h"
#include "../Entities.h"
#include "../GameContext.h"
#include "../SceneGraph.h"
#include "../typedef.h"

#include <math/Vector.h>
#include <snacman/Profiling.h>
#include <snacman/QueryManipulation.h>
#include <utility>

namespace ad {
namespace snacgame {
namespace system {

PlayerSpawner::PlayerSpawner(GameContext & aGameContext) :
    mGameContext{&aGameContext},
    mPlayerSlots{mGameContext->mWorld},
    mPlayers{mGameContext->mWorld},
    mSpawner{mGameContext->mWorld}
{}

void PlayerSpawner::spawnPlayersBeforeRound(EntHandle aLevel)
{
    TIME_SINGLE(Main, "Spawn before round");

    std::array<EntHandle, 4> spawnedSlots;
    std::size_t spawnedPlayerCount = 0;
    Phase ouais;
    mPlayerSlots.each([this, &ouais, &aLevel, &spawnedSlots, &spawnedPlayerCount](
                          EntHandle aSlotHandle,
                          const component::PlayerSlot &) {
        component::PlayerSlot & slot =
            snac::getComponent<component::PlayerSlot>(aSlotHandle);
        EntHandle spawnHandle = snac::getFirstHandle(
            mSpawner, [](const component::Spawner & aSpawner) {
                return !aSpawner.mSpawnedPlayer;
            });
        if (spawnHandle.isValid() && !slot.mPlayer.isValid())
        {
            component::Spawner & spawner =
                snac::getComponent<component::Spawner>(spawnHandle);
            math::Position<3, float> spawnPosition{
                spawner.mSpawnPosition.x(),
                spawner.mSpawnPosition.y(),
                20.f + 5.f * slot.mSlotIndex,
            };
            slot.mPlayer = createInGamePlayer(*mGameContext, aSlotHandle,
                                              spawnPosition);
            slot.mPlayer.get(ouais)
                ->add(component::Speed{{0.f, 0.f, 0.f}})
                .add(component::Gravity{gPlayerHeight});

            insertEntityInScene(slot.mPlayer, aLevel);
            const component::SceneNode & sceneNode = snac::getComponent<component::SceneNode>(slot.mPlayer);
            updateGlobalPosition(sceneNode);

            spawner.mSpawnedPlayer = true;
            spawnedSlots.at(spawnedPlayerCount++) = aSlotHandle;
        }
    });

    Phase spawnPlayer;
    for (std::size_t i = 0; i < spawnedPlayerCount; i++)
    {
        snac::removeComponent<component::Controller>(spawnedSlots.at(i));
    }
}

void PlayerSpawner::spawnPlayersDuringRound(EntHandle aLevel)
{
    TIME_RECURRING_CLASSFUNC(Main);

    std::array<EntHandle, 4> spawnedSlots;
    std::size_t spawnedPlayerCount = 0;
    mPlayerSlots.each(
        [this, &aLevel, &spawnedSlots, &spawnedPlayerCount](EntHandle aSlotHandle,
                                        const component::PlayerSlot &) {
            component::PlayerSlot & slot =
                snac::getComponent<component::PlayerSlot>(aSlotHandle);
            EntHandle spawnHandle = snac::getFirstHandle(
                mSpawner, [](const component::Spawner & aSpawner) {
                    return !aSpawner.mSpawnedPlayer;
                });
            if (spawnHandle.isValid() && !slot.mPlayer.isValid())
            {
                component::Spawner & spawner =
                    snac::getComponent<component::Spawner>(spawnHandle);
                slot.mPlayer = createInGamePlayer(*mGameContext, aSlotHandle,
                                                  spawner.mSpawnPosition);

                insertEntityInScene(slot.mPlayer, aLevel);
                updateGlobalPosition(
                    snac::getComponent<component::SceneNode>(slot.mPlayer));

                spawner.mSpawnedPlayer = true;
                spawnedSlots.at(spawnedPlayerCount++) = aSlotHandle;
            }
        });

    {
        Phase spawnPlayer;
        for (EntHandle spawnHandle : spawnedSlots)
        {
            snac::removeComponent<component::Controller>(spawnHandle);
        }
    }

    //TODO(franz): remove from playerSpawner 
    std::vector<std::pair<EntHandle, component::PlayerRoundData *>> leaders;
    int leaderScore = 1;
    mPlayers.each(
        [&leaders, &leaderScore](EntHandle aHandle,
                                 component::PlayerRoundData & aRoundData) {
            component::PlayerGameData & gameData =
                snac::getComponent<component::PlayerGameData>(aRoundData.mSlot);
            if (!aRoundData.mCrown.isValid())
            {
                if (gameData.mRoundsWon > leaderScore)
                {
                    leaders.clear();
                    leaders.push_back({aHandle, &aRoundData});
                    leaderScore = gameData.mRoundsWon;
                }
                else if (gameData.mRoundsWon == leaderScore)
                {
                    leaders.push_back({aHandle, &aRoundData});
                }
            }
        });

    for (auto & [leaderEnt, leaderData] : leaders)
    {
        EntHandle crown = createCrown(*mGameContext);
        leaderData->mCrown = crown;
        insertEntityInScene(crown, leaderEnt);
        updateGlobalPosition(snac::getComponent<component::SceneNode>(crown));
    }
}

} // namespace system
} // namespace snacgame
} // namespace ad
