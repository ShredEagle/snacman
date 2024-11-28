#include "BurgerLossSystem.h"

#include "math/Color.h"
#include "snac-renderer-V1/DebugDrawer.h"
#include "snacman/simulations/snacgame/Entities.h"
#include "snacman/simulations/snacgame/SceneGraph.h"
#include "snacman/simulations/snacgame/component/Collision.h"
#include "snacman/simulations/snacgame/component/GlobalPose.h"
#include "snacman/simulations/snacgame/GameContext.h"
#include "snacman/simulations/snacgame/component/Tags.h"

#include <snacman/DebugDrawing.h>

namespace ad {
namespace snacgame {
namespace system {

BurgerLoss::BurgerLoss(GameContext & aContext) :
    mBurgerHitbox{aContext.mWorld}, mBurgerParticles{aContext.mWorld},
    mContext{&aContext}
{}

void BurgerLoss::update(ent::Handle<ent::Entity> aLevel)
{
    ent::Phase destroyHitbox;
    mBurgerHitbox.each([this, &destroyHitbox, aLevel](ent::Handle<ent::Entity> aHandleHb, const component::BurgerLossHitbox & aHitbox,
                              const component::Collision & aCollision,
                              const component::GlobalPose & aPose) {
        const math::Box<float> hitbox =
            component::transformHitbox(aPose.mPosition, aCollision.mHitbox);
        component::LevelTile * tile = aHitbox.mTile;
        DBGDRAW(snac::gHitboxDrawer, snac::DebugDrawer::Level::debug).addBox(
            snac::DebugDrawer::Entry{
                .mPosition = {0.f, 0.f, 0.f},
                .mColor = math::hdr::Rgb_f{1.f, 0.5f, 0.1f},
            },
            hitbox
        );
        mBurgerParticles.each([&hitbox, &destroyHitbox, this, tile, &aHandleHb, aLevel](ent::Handle<ent::Entity> aHandleParticle, const component::BurgerParticle & aParticle,
                                 const component::Collision & aCollisionP,
                                 const component::GlobalPose & aPoseP) {
            const math::Box<float> hitboxParticles = component::transformHitbox(
                aPoseP.mPosition, aCollisionP.mHitbox);
            bool collide = component::collideWithSat(hitbox, hitboxParticles);
            DBGDRAW(snac::gHitboxDrawer, snac::DebugDrawer::Level::debug).addBox(
                snac::DebugDrawer::Entry{
                    .mPosition = {0.f, 0.f, 0.f},
                    .mColor = math::hdr::Rgb_f{1.f, 0.5f, 0.1f},
                },
                hitboxParticles
            );
            if (collide)
            {
                if (tile)
                {
                    ent::Handle<ent::Entity> pill;
                    {
                        ent::Phase createPillPhase;
                        pill = createPill(*mContext, createPillPhase, aParticle.mTargetPos.xy());
                    }
                    insertEntityInScene(pill, aLevel);
                    tile->mPill = pill;
                }
                aHandleHb.get(destroyHitbox)->erase();
                aHandleParticle.get(destroyHitbox)->erase();
            }
        });
    });
}
} // namespace system
} // namespace snacgame
} // namespace ad
