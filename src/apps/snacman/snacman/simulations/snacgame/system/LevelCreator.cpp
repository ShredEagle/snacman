#include "LevelCreator.h"

#include "entity/Entity.h"
#include "markovjunior/Interpreter.h"

#include "../component/LevelData.h"

namespace ad {
namespace snacgame {
namespace system {

void LevelCreator::update()
{
    ent::Phase createLevelPhase;
    mCreatable.each([&](ent::Handle<ent::Entity> aHandle,
                        component::LevelData & aLevelData) {
        markovjunior::Interpreter interpreter(
            aLevelData.mAssetRoot, aLevelData.mFile, aLevelData.mSize,
            aLevelData.mSeed);

        interpreter.setup();

        int stepPerformed = 0;

        // TODO: make this better and maybe a function in markov
        while (interpreter.mCurrentBranch != nullptr && stepPerformed > -1)
        {
            interpreter.runStep();
            stepPerformed++;
        }

        if (interpreter.mCurrentBranch != nullptr)
        {
            return;
        }

        markovjunior::Grid aGrid = interpreter.mGrid;

        aLevelData.mTiles.reserve(aLevelData.mSize.width()
                                  * aLevelData.mSize.height());
        std::vector<component::Tile> & tiles = aLevelData.mTiles;
        int rowCount = aLevelData.mSize.width();
        int colCount = aLevelData.mSize.height();

        for (int z = 0; z < aGrid.mSize.depth(); z++)
        {
            for (int y = 0; y < aGrid.mSize.height(); y++)
            {
                for (int x = 0; x < aGrid.mSize.width(); x++)
                {
                    unsigned char value = aGrid.mCharacters.at(
                        aGrid.mState.at(aGrid.getFlatGridIndex({x, y, z})));
                    switch (value)
                    {
                    case 'W':
                        tiles.push_back(component::Tile{.mType = component::TileType::Path});
                        createPathEntity(*mWorld, createLevelPhase,
                                         math::Position<2, int>{x, y});
                        createPill(*mWorld, createLevelPhase,
                                   math::Position<2, int>{x, y});
                        break;
                    case 'K':
                        tiles.push_back(component::Tile{.mType = component::TileType::Portal});
                        createPortalEntity(*mWorld, createLevelPhase,
                                           math::Position<2, int>{x, y});
                        break;
                    case 'O':
                        tiles.push_back(component::Tile{.mType = component::TileType::Pen});
                        createCopPenEntity(*mWorld, createLevelPhase,
                                           math::Position<2, int>{x, y});
                        break;
                    case 'U':
                        tiles.push_back(component::Tile{.mType = component::TileType::Spawn});
                        createPlayerSpawnEntity(*mWorld, createLevelPhase,
                                                math::Position<2, int>{x, y});
                        break;
                    default:
                        tiles.push_back(component::Tile{.mType = component::TileType::Void});
                        break;
                    }
                }
            }
        }

        for (int i = 1; i < colCount - 1; ++i)
        {
            for (int j = 1; j < rowCount - 1; ++j)
            {
                component::Tile & tile = tiles.at(i + j * colCount);

                if (tiles.at(i + (j + 1) * colCount).mType
                    != component::TileType::Void)
                {
                    tile.mAllowedMove |= component::gAllowedMovementDown;
                }
                if (tiles.at(i + (j - 1) * colCount).mType
                    != component::TileType::Void)
                {
                    tile.mAllowedMove |= component::gAllowedMovementUp;
                }
                if (tiles.at((i + 1) + j * colCount).mType
                    != component::TileType::Void)
                {
                    tile.mAllowedMove |= component::gAllowedMovementLeft;
                }
                if (tiles.at((i - 1) + j * colCount).mType
                    != component::TileType::Void)
                {
                    tile.mAllowedMove |= component::gAllowedMovementRight;
                }
            }
        }

        aHandle.get(createLevelPhase)->remove<component::LevelToCreate>();
        aHandle.get(createLevelPhase)->add(component::LevelCreated{});
    });
}

} // namespace system
} // namespace snacgame
} // namespace ad
