#pragma once

#include <entity/Query.h>

#include <handy/random.h>

namespace ad {
namespace snacgame {
struct GameContext;
namespace component {
struct LevelTile;
struct Portal;
struct Geometry;
struct LevelSetupData;
struct RoundTransient;
}
namespace system {

class LevelManager
{
public:
    LevelManager(GameContext & aGameContext);

    void update();

    void destroyTilesEntities();

    ent::Handle<ent::Entity> createLevel(const component::LevelSetupData & aSetupData);

private:
    GameContext * mGameContext;
    ent::Query<component::LevelTile> mTiles;
    ent::Query<component::RoundTransient> mPowerUpAndPills;
    ent::Query<component::Portal, component::Geometry> mPortals;

    static constexpr int gMaxRand = 65536;
    Randomizer<> mRandom{0, gMaxRand};

    static constexpr float gMinPowerUpPeriod = 0.4f;
    static constexpr float gMaxPowerUpPeriod = 1.4f;
};

} // namespace system
} // namespace snacgame
} // namespace ad
