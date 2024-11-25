#include "LevelManager.h"

#include "../component/GlobalPose.h"
#include "../component/LevelData.h"
#include "../component/Portal.h"
#include "../component/SceneNode.h"
#include "../component/Tags.h"
#include "../component/PlayerGameData.h"
#include "../component/PlayerSlot.h"

#include "../Entities.h"
#include "../GameContext.h"
#include "../LevelHelper.h"
#include "../SceneGraph.h"
#include "../typedef.h"

#include <entity/Entity.h>
#include <markovjunior/Interpreter.h>
#include <optional>
#include <snacman/Profiling.h>
#include <vector>

namespace ad {
namespace snacgame {
namespace system {

constexpr Pos3 gBaseLevelPos{-7.f, -7.f, 0.f};
constexpr float gBaseLevelScalingNumerator = 15.f;

LevelManager::LevelManager(GameContext & aGameContext) :
    mGameContext{&aGameContext},
    mTiles{mGameContext->mWorld},
    mPowerUpAndPills{mGameContext->mWorld},
    mPortals{mGameContext->mWorld}
{}

void LevelManager::destroyTilesEntities()
{
    Phase destroyTiles;
    mTiles.each(
        [&destroyTiles](EntHandle aTileHandle, const component::LevelTile &)
        { aTileHandle.get(destroyTiles)->erase(); });
}

EntHandle
LevelManager::createLevel(const component::LevelSetupData & aSetupData)
{
    EntHandle level = mGameContext->mWorld.addEntity();

    math::Size<3, int> levelSize = aSetupData.mAvailableSizes.at(0);
    component::Level levelData{
        .mSize = levelSize,
    };
    markovjunior::Interpreter interpreter(
        aSetupData.mAssetRoot, aSetupData.mFile, levelSize, aSetupData.mSeed);

    interpreter.setup();
    {
        TIME_SINGLE(Main, "Markov_steps");
        while (interpreter.mCurrentBranch != nullptr)
        {
            interpreter.runStep();
        }
    }

    TIME_SINGLE(Main, "Level_instantiation");

    int stride = levelData.mSize.width();
    int height = levelData.mSize.height();
    markovjunior::Grid aGrid = interpreter.mGrid;
    std::vector<component::Tile> tiles;
    tiles.reserve(stride * height);
    std::vector<component::PathfindNode> nodes;
    nodes.reserve(stride * height);
    std::vector<int> portalIndices;

    // This takes a lot of time
    // Presumable because there is a lot of allocation and moving
    // of archetypes
    // However this needs a way to instantiate null handle
    // so that we can pre instantiate component containing handles
    // Or we need a custom allocator for the whole entity manager
    // Right now it's 15 ms in release for 225 entity that's 66us per
    // instantiation
    {
        TIME_SINGLE(Main, "Create elements with phase destruction");
        {
            Phase createLevel;

            // There is no 3rd dimension right now
            int z = 0;
            
            {
            TIME_SINGLE(Main, "Create elements no phase destruction");
            for (int y = 0; y < aGrid.mSize.height(); y++)
            {
                for (int x = 0; x < aGrid.mSize.width(); x++)
                {
                    float xFloat = static_cast<float>(x);
                    float yFloat = static_cast<float>(y);
                    size_t flatIndex = aGrid.getFlatGridIndex({x, y, z});
                    unsigned char value =
                        aGrid.mCharacters.at(aGrid.mState.at(flatIndex));
                    nodes.push_back({
                        .mIndex = flatIndex,
                        .mPos = math::Position<2, float>{static_cast<float>(x),
                                                         static_cast<float>(y)},
                        .mPathable = value != 'B',
                    });
                    component::TileType type = component::TileType::Void;
                    Pos2 tilePos{xFloat, yFloat};

                    switch (value)
                    {
                    case 'W':
                    {
                        type = component::TileType::Path;
                        createPathEntity(*mGameContext, createLevel, tilePos);
                        createPill(*mGameContext, createLevel, tilePos);
                        break;
                    }
                    case 'O':
                    {
                        type = component::TileType::Powerup;
                        createPathEntity(*mGameContext, createLevel, tilePos);

                        using component::PowerUpType;
                        createPowerUp(
                            *mGameContext, createLevel, tilePos,
                            static_cast<PowerUpType>(
                                mRandom() % static_cast<int>(PowerUpType::None)),
                            gMinPowerUpPeriod
                                + (gMaxPowerUpPeriod - gMinPowerUpPeriod)
                                      * ((float)mRandom() / (float) gMaxRand));
                        break;
                    }
                    case 'K':
                    {
                        type = component::TileType::Portal;
                        portalIndices.push_back((int)flatIndex);
                        createPortalEntity(*mGameContext, createLevel, tilePos,
                                           (int)flatIndex);
                        break;
                    }
                    case 'F':
                    {
                        type = component::TileType::Spawn;
                        createPlayerSpawnEntity(*mGameContext, createLevel,
                                                tilePos);
                        break;
                    }
                    default:
                        break;
                    }

                    tiles.push_back(component::Tile{
                        .mType = type,
                        .mPos = tilePos,
                    });
                }
            }
            }

            /* createAnimatedTest(*mGameContext, createLevel, snac::Clock::now(),
             * {14.f, 7.f}); */
            /* createAnimatedTest(*mGameContext, createLevel, snac::Clock::now() +
             * snac::ms{1000}, { 0.f, 7.f}); */

            {
            TIME_SINGLE(Main, "Allow movement setup");
            for (int i = 1; i < height - 1; ++i)
            {
                for (int j = 1; j < stride - 1; ++j)
                {
                    component::Tile & tile = tiles.at(i + j * stride);

                    for (size_t moveIndex = 0; moveIndex < gDirections.size();
                         ++moveIndex)
                    {
                        const Vec2_i dir = gDirections.at(moveIndex);
                        const int move = gAllowedMovement.at(moveIndex);
                        if (tiles.at(i + dir.x() + (j + dir.y()) * stride).mType
                            != component::TileType::Void)
                        {
                            tile.mAllowedMove |= move;
                            if (tile.mType == component::TileType::Portal)
                            {
                                // This inverse the allowed Movement flag up becomes
                                // down and right becomes left because this flip the
                                // least significant bit
                                tile.mAllowedMove |=
                                    gAllowedMovement.at(moveIndex ^ 0b01);
                            }
                        }
                    }
                }
            }
            }

            levelData.mTiles = std::move(tiles);
            levelData.mNodes = std::move(nodes);
            levelData.mPortalIndex = std::move(portalIndices);

            level.get(createLevel)
                ->add(levelData)
                .add(component::SceneNode{})
                .add(component::Geometry{
                    .mPosition = gBaseLevelPos,
                    .mScaling = gBaseLevelScalingNumerator
                                / (float) std::max(stride, height)})
                .add(component::GlobalPose{});
        } // End createLevel Phase
    }

    {
    TIME_SINGLE(Main, "Scene insertion");
    mTiles.each([level](EntHandle aTileHandle, const component::LevelTile & aTile)
                { insertEntityInScene(aTileHandle, level); });
    mPowerUpAndPills.each([level](EntHandle aTileHandle, const component::RoundTransient &)
                { insertEntityInScene(aTileHandle, level); });
    }

    {
    TIME_SINGLE(Main, "Portal info creation");
    mPortals.each(
        [&tiles = levelData.mTiles, stride, &level, this](EntHandle aHandle,
                                       component::Portal & aPortal,
                                       const component::Geometry & aGeo)
        {
        Pos2_i pos = {(int) aGeo.mPosition.x(), (int) aGeo.mPosition.y()};

        // HACK: (franz)
        // The algorithm only checks for horizontal orientation of the portal
        if (tiles.at(pos.x() + 1 + pos.y() * stride).mType
            != component::TileType::Void)
        {
            addPortalInfo(*mGameContext, level, aPortal, aGeo,
                          Vec3{-1.f, 0.f, 0.f});
        }
        else if (tiles.at(pos.x() - 1 + pos.y() * stride).mType
                 != component::TileType::Void)
        {
            addPortalInfo(*mGameContext, level, aPortal, aGeo,
                          Vec3{1.f, 0.f, 0.f});
        }
    });
    }

    return level;
}

} // namespace system
} // namespace snacgame
} // namespace ad
