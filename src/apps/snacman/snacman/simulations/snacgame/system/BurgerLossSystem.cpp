#include "BurgerLossSystem.h"

#include "math/Color.h"
#include "snac-renderer-V1/DebugDrawer.h"
#include "snacman/EntityUtilities.h"
#include "snacman/Logging.h"
#include "snacman/Timing.h"
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

void BurgerLoss::update(ent::Handle<ent::Entity> aLevel, const snac::Time & aTime)
{
    ent::Phase destroyHitbox;
    //TODO(franz): this isn't right particle should have the target they aim for
    // we don't have to check all hitbox
    std::vector<ent::Handle<ent::Entity>> usedTiles;
    usedTiles.reserve(mBurgerHitbox.countMatches());
    mBurgerHitbox.each([this, &destroyHitbox, aLevel, &aTime, &usedTiles](ent::Handle<ent::Entity> aHandleHb,
                              const component::BurgerLossHitbox & aHitbox,
                              const component::Collision & aCollision,
                              const component::GlobalPose & aPose) {
        const math::Box<float> hitbox =
            component::transformHitbox(aPose.mPosition, aCollision.mHitbox);
        ent::Handle<ent::Entity> tileHandle = aHitbox.mTile;
        DBGDRAW(snac::gHitboxDrawer, snac::DebugDrawer::Level::debug).addBox(
            snac::DebugDrawer::Entry{
                .mPosition = {0.f, 0.f, 0.f},
                .mColor = math::hdr::Rgb_f{1.f, 0.5f, 0.1f},
            },
            hitbox
        );
        mBurgerParticles.each([&hitbox, &destroyHitbox, this, &tileHandle, &aHandleHb, aLevel, &aTime, &usedTiles](
                        ent::Handle<ent::Entity> aHandleParticle,
                        const component::BurgerParticle & aParticle,
                        const component::Collision & aCollisionP,
                        const component::GlobalPose & aPoseP) {
            const math::Box<float> hitboxParticles = component::transformHitbox(
                aPoseP.mPosition, aCollisionP.mHitbox);
            bool collide = component::collideWithSat(hitbox, hitboxParticles);
            const double duration = snac::asSeconds(aTime.mTimepoint - aParticle.mStartTime);
            DBGDRAW(snac::gHitboxDrawer, snac::DebugDrawer::Level::debug).addBox(
                snac::DebugDrawer::Entry{
                    .mPosition = {0.f, 0.f, 0.f},
                    .mColor = math::hdr::Rgb_f{1.f, 0.5f, 0.1f},
                },
                hitboxParticles
            );
            //TODO(franz): I mean wtf is this get your shit together
            bool notDone = std::find(usedTiles.begin(), usedTiles.end(), tileHandle) == usedTiles.end() &&
                               std::find(usedTiles.begin(), usedTiles.end(), aHandleParticle) == usedTiles.end();
            if ((collide ||
                duration > gBurgerTimeTotarget) &&
                notDone)
            {
                if (tileHandle.isValid())
                {
                    component::LevelTile & tile = snac::getComponent<component::LevelTile>(tileHandle);
                    ent::Handle<ent::Entity> pill;
                    {
                        ent::Phase createPillPhase;
                        pill = createPill(*mContext, createPillPhase, aParticle.mTargetPos.xy());
                    }
                    insertEntityInScene(pill, aLevel);
                    tile.mPill = pill;
                }
                usedTiles.push_back(tileHandle);
                usedTiles.push_back(aHandleParticle);
                SELOG(debug)("Tile handle: {}",tileHandle.id());
                SELOG(debug)("hb handle: {}",aHandleHb.id());
                SELOG(debug)("Particle handle: {}",aHandleParticle.id());
                aHandleHb.get(destroyHitbox)->erase();
                aHandleParticle.get(destroyHitbox)->erase();
            }
        });
    });
}
} // namespace system
} // namespace snacgame
} // namespace ad
