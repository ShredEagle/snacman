#include "Entities.h"

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
  mWorld.addEntity().get(aInit)->add(component::Geometry{
      .mPosition = aPos,
      .mYRotation = math::Degree<float>{0.f},
      .mColor = math::hdr::gYellow<float>});
}
} // namespace snacgame
} // namespace ad
