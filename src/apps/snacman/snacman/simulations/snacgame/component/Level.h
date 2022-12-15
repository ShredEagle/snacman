#pragma once

#include "entity/Entity.h"
#include "entity/EntityManager.h"
#include "markovjunior/Grid.h"
#include "math/Matrix.h"

#include "../Entities.h"

namespace ad {
namespace snacgame {
namespace component {

struct Level
{
    // Level grid stored as
    // 0 1 2 ... mColCount - 1 (0th row)
    // mColCount mColCount + 1 ... 2 * mColCount - 1 (1th row)
    // ...
    // mColCount * (mRowCount - 1) ... (mColCount * mRowCount) - 1 (mRowCountth
    // row)
    Level(ent::EntityManager & aWorld,
          ent::Phase & aInit,
          const markovjunior::Grid & aGrid) :
        mRowCount{aGrid.mSize.height()}, mColCount{aGrid.mSize.width()}
    {
        mLevelGrid.reserve(mRowCount * mColCount);

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
                        mLevelGrid.push_back(createPathEntity(
                            aWorld, aInit,
                            math::Position<2, int>{
                                x,
                                y}));
                        break;
                    case 'K':
                        mLevelGrid.push_back(createPortalEntity(
                            aWorld, aInit,
                            math::Position<2, int>{
                                x,
                                y}));
                        break;
                    case 'O':
                        mLevelGrid.push_back(createCopPenEntity(
                            aWorld, aInit,
                            math::Position<2, int>{
                                x,
                                y}));
                        break;
                    case 'U':
                        mLevelGrid.push_back(createPlayerSpawnEntity(
                            aWorld, aInit,
                            math::Position<2, int>{
                                x,
                                y}));
                        break;
                    default:
                        mLevelGrid.push_back(aWorld.addEntity());
                        break;
                    }
                }
            }
        }
    }
    // 0 ... 28
    // 29 ... 57
    // 58 59 60

    std::vector<ent::Handle<ent::Entity>> mLevelGrid;
    int mRowCount;
    int mColCount;
    float mCellSize = 2.f;
};

} // namespace component
} // namespace snacgame
} // namespace ad
