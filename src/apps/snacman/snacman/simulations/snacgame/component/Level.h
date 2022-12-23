#pragma once

#include "AllowedMovement.h"
#include "../Entities.h"

#include "entity/Entity.h"
#include "entity/EntityManager.h"
#include "markovjunior/Grid.h"
#include "math/Matrix.h"

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

        {
            ent::Phase createLevelPhase;
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
                                aWorld, createLevelPhase,
                                math::Position<2, int>{
                                    x,
                                    y}));
                            createPill(aWorld, createLevelPhase,
                                math::Position<2, int>{
                                    x,
                                    y});
                            break;
                        case 'K':
                            mLevelGrid.push_back(createPortalEntity(
                                aWorld, createLevelPhase,
                                math::Position<2, int>{
                                    x,
                                    y}));
                            break;
                        case 'O':
                            mLevelGrid.push_back(createCopPenEntity(
                                aWorld, createLevelPhase,
                                math::Position<2, int>{
                                    x,
                                    y}));
                            break;
                        case 'U':
                            mLevelGrid.push_back(createPlayerSpawnEntity(
                                aWorld, createLevelPhase,
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

        for (int i = 1; i < mColCount - 1; ++i)
        {
            for (int j = 1; j < mRowCount - 1; ++j)
            {
                auto entity = mLevelGrid.at(i + j * mColCount).get(aInit);
                if (entity->has<component::Geometry>())
                {
                    int allowed = gAllowedMovementNone;

                    if (mLevelGrid
                            .at(i
                                + (j + 1) * mColCount)
                            .get(aInit)
                            ->has<component::Geometry>())
                    {
                        allowed |= gAllowedMovementDown;
                    }
                    if (mLevelGrid
                            .at(i
                                + (j - 1) * mColCount)
                            .get(aInit)
                            ->has<component::Geometry>())
                    {
                        allowed |= gAllowedMovementUp;
                    }
                    if (mLevelGrid
                            .at((i + 1)
                                + j * mColCount)
                            .get(aInit)
                            ->has<component::Geometry>())
                    {
                        allowed |= gAllowedMovementLeft;
                    }
                    if (mLevelGrid
                            .at((i - 1)
                                + j * mColCount)
                            .get(aInit)
                            ->has<component::Geometry>())
                    {
                        allowed |= gAllowedMovementRight;
                    }

                    entity->add(component::AllowedMovement{allowed});
                }
            }
        }
    }

    std::vector<ent::Handle<ent::Entity>> mLevelGrid;
    int mRowCount;
    int mColCount;
    float mCellSize = 2.f;
};

} // namespace component
} // namespace snacgame
} // namespace ad
