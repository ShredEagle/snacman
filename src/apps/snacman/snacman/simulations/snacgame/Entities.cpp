#include "Entities.h"
#include "math/Color.h"
#include "snacman/simulations/snacgame/component/PlayerLifeCycle.h"
#include "snacman/simulations/snacgame/component/Spawner.h"

namespace ad {
namespace snacgame {

void createPathEntity(ent::EntityManager & mWorld, ent::Phase & aInit, const math::Position<3, float> & aPos)
{
  mWorld.addEntity().get(aInit)->add(component::Geometry{
      .mPosition = aPos,
      .mYRotation = math::Degree<float>{0.f},
      .mColor = math::hdr::gWhite<float>});
}

void createPortalEntity(ent::EntityManager & mWorld, ent::Phase & aInit, const math::Position<3, float> & aPos)
{
  mWorld.addEntity().get(aInit)->add(component::Geometry{
      .mPosition = aPos,
      .mYRotation = math::Degree<float>{0.f},
      .mColor = math::hdr::gRed<float>});
}

void createCopPenEntity(ent::EntityManager & mWorld, ent::Phase & aInit, const math::Position<3, float> & aPos)
{
  mWorld.addEntity().get(aInit)->add(component::Geometry{
      .mPosition = aPos,
      .mYRotation = math::Degree<float>{0.f},
      .mColor = math::hdr::gYellow<float>});
}

void createPlayerSpawnEntity(ent::EntityManager & mWorld, ent::Phase & aInit, const math::Position<3, float> & aPos)
{
  auto spawner = mWorld.addEntity().get(aInit);
  spawner->add(component::Geometry{
      .mPosition = aPos,
      .mYRotation = math::Degree<float>{0.f},
      .mColor = math::hdr::gCyan<float>});
  spawner->add(component::Spawner{
      .mSpawnPosition = aPos + math::Vec<3, float>{0.f, 2.f, 0.f}});
}

void createPlayerEntity(ent::EntityManager & mWorld, ent::Phase & aInit)
{
    auto playerEntity = mWorld.addEntity().get(aInit);
    playerEntity->add(component::Geometry{
      .mPosition = math::Position<3, float>::Zero(),
      .mYRotation = math::Degree<float>{0.f},
      .mColor = math::hdr::gMagenta<float>,
      .mShouldBeDrawn = false});
    playerEntity->add(component::PlayerLifeCycle{});
}
} // namespace snacgame
} // namespace ad
