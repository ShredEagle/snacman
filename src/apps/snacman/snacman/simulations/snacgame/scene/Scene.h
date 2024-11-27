#pragma once

#include "snacman/simulations/snacgame/component/PlayerGameData.h"
#include "snacman/simulations/snacgame/component/PlayerRoundData.h"
#include "snacman/simulations/snacgame/system/SystemOrbitalCamera.h"
#include "../component/SceneNode.h"
#include "../OrbitalControlInput.h"

#include <snacman/Logging.h>
#include <snacman/CompilerDef.h>
#include <snacman/serialization/Serial.h>
#include <reflexion/NameValuePair.h>

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>
#include <entity/Wrap.h>

#include <markovjunior/Grid.h>
#include <optional>
#include <stack>
#include <string>
#include <variant>

namespace ad {

namespace snacgame {
namespace component {
struct MappingContext;
}
namespace scene {

const inline std::string gQuitTransitionName = "quit";

struct RankingList
{
    using Rank = std::pair<ent::Handle<ent::Entity>, component::PlayerGameData>;
    std::array<Rank, 4> ranking;
    std::size_t playerCount = 0;

    void sort() {
        SNAC_GCC_PUSH_DIAG(array-bounds)
        std::sort(
            ranking.begin(),
            ranking.begin() + playerCount,
            [](RankingList::Rank & a, RankingList::Rank & b) -> bool {
                return a.second.mRoundsWon > b.second.mRoundsWon;
            }
        );
        SNAC_GCC_POP_DIAG()
    }

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

struct WinnerList
{
    std::array<std::pair<ent::Handle<ent::Entity>, component::PlayerRoundData *>, 4> leaders;
    int leaderCount = 0;
    int leaderScore = -1;
};

struct MenuSceneInfo
{

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

struct JoinGameSceneInfo
{
    int mTransitionControllerId = -1;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mTransitionControllerId));
    }
};

struct GameSceneInfo
{

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
    }
};

struct PodiumSceneInfo
{

    RankingList mRanking;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mRanking));
    }
};

struct DisconnectedControllerInfo
{
    std::array<int, 4> mDisconnectedControllerId;
    int mDisconnectedCount;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mDisconnectedControllerId));
        aWitness.witness(NVP(mDisconnectedCount));
    }
};

enum class TransType {
    MenuFromPause,          // 0
    JoinGameFromMenu,       // 1
    JoinGameFromPodium,     // 2
    JoinGameFromPause,      // 3
    GameFromJoinGame,       // 4
    GameFromPodium,         // 5
    GameFromPause,          // 6
    GameFromDiscWithKick,   // 7
    GameFromDiscWitoutKick, // 8
    PauseFromGame,          // 9
    PodiumFromGame,         // 10
    QuitAll,                // 11
    QuitToMenu,             // 12
    DisconnectedFromGame,   // 13
    Count,                  // 14
};

struct Transition
{
    TransType mTransitionType;
    bool shouldTeardown = true;
    bool shouldSetup = true;
    std::variant<MenuSceneInfo, JoinGameSceneInfo, GameSceneInfo, DisconnectedControllerInfo, PodiumSceneInfo> mSceneInfo;

    bool operator==(const Transition & aRhs) const
    {
        return mTransitionType == aRhs.mTransitionType;
    }

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mTransitionType));
        aWitness.witness(NVP(shouldTeardown));
        aWitness.witness(NVP(shouldSetup));
        aWitness.witness(NVP(mSceneInfo));
    }
};

constexpr std::size_t MAX_SCENE_ENTITY = 10000;

struct SceneHandleNode
{
    ent::Handle<ent::Entity> mHandle;
    SceneHandleNode * mPrev = nullptr;
    SceneHandleNode * mNext = nullptr;
};

// This is a DLL stored in an array with a max size of
// MAX_SCENE_ENTITY
struct SceneEntityList
{
    std::array<SceneHandleNode, MAX_SCENE_ENTITY> mNodes;
    std::stack<
        std::size_t
        > mFreeList;
    std::size_t mLastIndex = 0;

    SceneEntityList() = default;
    SceneEntityList(SceneEntityList &&) = delete;
    SceneEntityList & operator=(const SceneEntityList &) = default;
    SceneEntityList & operator=(SceneEntityList &&) = delete;
    SceneEntityList(SceneEntityList &) = delete;
    SceneEntityList(const SceneEntityList &) = delete;

    ent::Handle<ent::Entity> addEntity(
        ent::EntityManager & aWorld,
        const std::string & aName
    )
    {
        assert(mLastIndex < MAX_SCENE_ENTITY && "Max entity in scene reached");
        SceneHandleNode newNode;
        newNode.mHandle = aWorld.addEntity(aName);
        std::size_t insertPos = mLastIndex;

        if (mFreeList.size() > 0)
        {
            insertPos = mFreeList.top();
            mFreeList.pop();
        }
        else
        {
            mLastIndex += 1;
        }

        if (insertPos != 0)
        {
            SceneHandleNode * prevNode = &mNodes.at(insertPos - 1);
            if (insertPos < mLastIndex)
            {
                newNode.mNext = prevNode->mNext;
            }
            newNode.mPrev = prevNode;

            // Since we're placing into an array this works 
            // like an index and we can point prevNode->mNext
            // to the insertPos of newNode before inserting
            // newNode (this assumes this function is atomic)
            prevNode->mNext = &mNodes.at(insertPos);
        }
        else if (insertPos < mLastIndex)
        {
            newNode.mNext = &mNodes.at(1);
        }

        mNodes.at(insertPos) = newNode;

        return newNode.mHandle;
    }

    void removeEntity(ent::Handle<ent::Entity> aHandle)
    {
        SceneHandleNode * current = &mNodes.at(0);
        for (;current != nullptr;current = current->mNext)
        {
            if (current->mHandle == aHandle)
            {
                mFreeList.push(current - &mNodes.at(0));
                current->mPrev->mNext = current->mNext;
                current->mNext->mPrev = current->mPrev;
                break;
            }
        }

#ifndef NDEBUG
        if (current == nullptr)
        {
            SELOG(error)("Tried to remove {} from an entity list where it is not present", aHandle.name());
        }
#endif
    }

    ~SceneEntityList()
    {
        ent::Phase destroy;
        SceneHandleNode * current = &mNodes.at(0);
        for (;current != nullptr;current = current->mNext)
        {
            auto handle = current->mHandle;
            if (handle.isValid())
            {
                handle.get(destroy)->erase();
            }
        }
    }
};

} // namespace scene
} // namespace snacgame
} // namespace ad

namespace std {
template <>
struct hash<ad::snacgame::scene::Transition>
{
    size_t operator()(const ad::snacgame::scene::Transition & aTransition) const
    {
        return hash<ad::snacgame::scene::TransType>()(aTransition.mTransitionType);
    }
};
}; // namespace std

namespace ad {

namespace snac {
struct Time;
class Orbital;
}

struct RawInput;

namespace snacgame {

struct GameContext;

namespace scene {

class Scene
{
public:
    // TODO :(franz) make a cpp file please
    Scene(std::string aName,
          GameContext & aGameContext,
          ent::Wrap<component::MappingContext> & aContext);
    Scene(const Scene &) = default;
    Scene(Scene && aScene) noexcept;

    Scene & operator=(const Scene &) = delete;
    Scene & operator=(Scene &&) = delete;
    virtual ~Scene() = default;

    virtual void onExit(Transition aTransition) = 0;
    virtual void onEnter(Transition aTransition) = 0;
    virtual void update(const snac::Time & aTime, RawInput & aInput) = 0;

    std::string mName;

protected:
    GameContext & mGameContext;
    ent::Handle<ent::Entity> mSystems;
    std::vector<ent::Handle<ent::Entity>> mOwnedEntities;
    ent::Wrap<component::MappingContext> & mMappingContext;
    ent::Query<system::OrbitalCamera> mCameraQuery;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
