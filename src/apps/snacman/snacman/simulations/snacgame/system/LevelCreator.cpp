#include "LevelCreator.h"

#include "../Entities.h"
#include "../component/LevelData.h"

#include <snacman/Profiling.h>

#include <entity/Entity.h>

#include <markovjunior/Interpreter.h>

namespace ad {
namespace snacgame {
namespace system {

void LevelCreator::update(GameContext & aContext)
{
    TIME_RECURRING_CLASSFUNC(Main);

    ent::Phase createLevelPhase;
    mCreatable.each([&](ent::Handle<ent::Entity> aHandle,
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
                    float xFloat = static_cast<float>(x);
                    float yFloat = static_cast<float>(y);
                    unsigned char value = aGrid.mCharacters.at(
                        aGrid.mState.at(aGrid.getFlatGridIndex({x, y, z})));
                    switch (value)
                    {
                    case 'W':
                        tiles.push_back(component::Tile{.mType = component::TileType::Path});
                        createPathEntity(aContext, 
                                         createLevelPhase,
                                         math::Position<2, float>{xFloat, yFloat});
                        createPill(aContext,
                                   createLevelPhase,
                                   math::Position<2, float>{xFloat, yFloat});
                        break;
                    case 'K':
                        tiles.push_back(component::Tile{.mType = component::TileType::Portal});
                        createPortalEntity(aContext, createLevelPhase,
                                           math::Position<2, float>{xFloat, yFloat});
                        break;
                    case 'O':
                        tiles.push_back(component::Tile{.mType = component::TileType::Pen});
                        createCopPenEntity(aContext, createLevelPhase,
                                           math::Position<2, float>{xFloat, yFloat});
                        break;
                    case 'U':
                        tiles.push_back(component::Tile{.mType = component::TileType::Spawn});
                        createPlayerSpawnEntity(aContext, createLevelPhase,
                                                math::Position<2, float>{xFloat, yFloat});
                        break;
                    default:
                        tiles.push_back(component::Tile{
                            .mType = component::TileType::Void});
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
                    tile.mAllowedMove |= component::gAllowedMovementUp;
                }
                if (tiles.at(i + (j - 1) * colCount).mType
                    != component::TileType::Void)
                {
                    tile.mAllowedMove |= component::gAllowedMovementDown;
                }
                if (tiles.at((i + 1) + j * colCount).mType
                    != component::TileType::Void)
                {
                    tile.mAllowedMove |= component::gAllowedMovementRight;
                }
                if (tiles.at((i - 1) + j * colCount).mType
                    != component::TileType::Void)
                {
                    tile.mAllowedMove |= component::gAllowedMovementLeft;
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
