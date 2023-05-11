#include "LevelCreator.h"
#include "snacman/simulations/snacgame/LevelHelper.h"

#include "../Entities.h"
#include "../SceneGraph.h"
#include "../typedef.h"

#include "../component/LevelData.h"
#include "../component/SceneNode.h"

#include <entity/Entity.h>
#include <markovjunior/Interpreter.h>
#include <snacman/Profiling.h>

namespace ad {
namespace snacgame {
namespace system {

void LevelCreator::update()
{
    TIME_RECURRING_CLASSFUNC(Main);

    mCreatable.each([&](EntHandle aLevelHandle,
                        component::LevelData & aLevelData) {
        markovjunior::Interpreter interpreter(
            aLevelData.mAssetRoot, aLevelData.mFile, aLevelData.mSize,
            aLevelData.mSeed);

        interpreter.setup();

        int stepPerformed = 0;

        // TODO: make this better and maybe a function in markov
        {
            TIME_SINGLE(Main, "Markov_steps");
            while (interpreter.mCurrentBranch != nullptr && stepPerformed > -1)
            {
                interpreter.runStep();
                stepPerformed++;
            }
        }

        if (interpreter.mCurrentBranch != nullptr)
        {
            return;
        }

        TIME_SINGLE(Main, "Level_instantiation");

        createWorldText(*mGameContext,
                        "SNACMAN",
                        {
                            .mPosition = math::Position<3, GLfloat>{-10.f, 0.f, -6.f},
                            .mScaling = 3.f,
                            .mOrientation = math::Quaternion{
                                                math::UnitVec<3, float>::MakeFromUnitLength({0.f, 1.f, 0.f}),
                                                math::Degree<float>{45.f}
                                            },
                        });

        markovjunior::Grid aGrid = interpreter.mGrid;

        aLevelData.mTiles.reserve(aLevelData.mSize.width()
                                  * aLevelData.mSize.height());
        std::vector<component::Tile> & tiles = aLevelData.mTiles;
        std::vector<component::PathfindNode> & nodes = aLevelData.mNodes;
        std::vector<int> & portals = aLevelData.mPortalIndex;
        int stride = aLevelData.mSize.width();
        int height = aLevelData.mSize.height();
        tiles.reserve(stride * height);
        nodes.reserve(stride * height);

        for (int z = 0; z < aGrid.mSize.depth(); z++)
        {
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

                    switch (value)
                    {
                    case 'W':
                    {
                        type = component::TileType::Path;
                        break;
                    }
                    case 'O':
                    {
                        type = component::TileType::Powerup;
                        break;
                    }
                    case 'K':
                    {
                        type = component::TileType::Portal;
                        break;
                    }
                    case 'F':
                    {
                        type = component::TileType::Spawn;
                        break;
                    }
                    default:
                        break;
                    }

                    tiles.push_back(component::Tile{
                        .mType = type,
                        .mPos = Pos2{xFloat, yFloat},
                        });
                }
            }
        }

        // This takes a lot of time
        // Presumable because there is a lot of allocation and moving
        // of archetypes
        // However this needs a way to instantiate null handle
        // so that we can pre instantiate component containing handles
        // Or we need a custom allocator for the whole entity manager
        // Right now it's 15 ms in release for 225 entity that's 66us per
        // instantiation
        {
        Phase createLevel;

        createAnimatedTest(*mGameContext, createLevel, snac::Clock::now(), {14.f, 7.f});
        createAnimatedTest(*mGameContext, createLevel, snac::Clock::now() + snac::ms{1000}, { 0.f, 7.f});

        for (int i = 1; i < height - 1; ++i)
        {
            for (int j = 1; j < stride - 1; ++j)
            {
                component::Tile & tile = tiles.at(i + j * stride);

                switch (tile.mType)
                {
                    case component::TileType::Path:
                    {
                        createPathEntity(
                            *mGameContext,
                            createLevel,
                            tile.mPos);
                        createPill(
                            *mGameContext,
                            createLevel,
                            tile.mPos);
                        break;
                    }
                    case component::TileType::Powerup:
                    {
                        createPathEntity(
                            *mGameContext,
                            createLevel,
                            tile.mPos);
                        createPowerUp(
                            *mGameContext,
                            createLevel,
                            tile.mPos);
                        break;
                    }
                    case component::TileType::Portal:
                    {
                        int portalIndex = static_cast<int>(i + j * stride);
                        portals.push_back(portalIndex);
                        createPortalEntity(
                            *mGameContext,
                            createLevel,
                            tile.mPos,
                            portalIndex);
                        break;
                    }
                    case component::TileType::Spawn:
                    {
                        createPlayerSpawnEntity(
                            *mGameContext,
                            createLevel,
                            tile.mPos);
                        break;
                    }
                    case component::TileType::Void:
                    default:
                        break;
                }

                for (size_t moveIndex = 0; moveIndex < gDirections.size(); ++moveIndex)
                {
                    const Vec2_i dir = gDirections.at(moveIndex);
                    const int move = gAllowedMovement.at(moveIndex);
                    if (tiles.at(i + dir.x() + (j + dir.y()) * stride).mType != component::TileType::Void)
                    {
                        tile.mAllowedMove |= move;
                        if (tile.mType == component::TileType::Portal)
                        {
                            // This inverse the allowed Movement flag up becomes down and right becomes left
                            // because this flip the least significant bit
                            tile.mAllowedMove |= gAllowedMovement.at(moveIndex ^ 0b01);
                        }
                    }
                }
            }
        }
        } // end createLevel phase

        createStageDecor(*mGameContext);
        mEntities.each([&aLevelHandle](EntHandle aHandle, const component::LevelEntity &) {
            insertEntityInScene(aHandle, aLevelHandle);
        });

        mPortals.each([&tiles, stride](EntHandle aHandle, component::Portal & aPortal, const component::Geometry & aGeo) {
                Pos2_i pos = {(int)aGeo.mPosition.x(), (int)aGeo.mPosition.y()};

                // HACK: (franz)
                // The algorithm only checks for horizontal orientation of the portal
                if (tiles.at(pos.x() + 1 + pos.y() * stride).mType != component::TileType::Void)
                {
                    addPortalInfo(aPortal, aGeo, Vec3{-1.f, 0.f, 0.f});
                }
                else if (tiles.at(pos.x() - 1 + pos.y() * stride).mType != component::TileType::Void)
                {
                    addPortalInfo(aPortal, aGeo, Vec3{1.f, 0.f, 0.f});
                }

        });

        {
        ent::Phase tagLevelCreation;
        aLevelHandle.get(tagLevelCreation)->remove<component::LevelToCreate>();
        aLevelHandle.get(tagLevelCreation)->add(component::LevelCreated{});
        }
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
