#pragma once

#include "component/Controller.h"
#include "component/Geometry.h"
#include "component/PlayerSlot.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <markovjunior/Grid.h>

namespace ad {

// Forward
namespace snac {
    struct Font;
}

namespace snacgame {


struct GameContext;


void createPill(GameContext & aContext,
                ent::Phase & aInit,
                const math::Position<2, int> & Pos);
ent::Handle<ent::Entity> createPathEntity(GameContext & aContext,
                                          ent::Phase & aInit,
                                          const math::Position<2, int> & aPos);
ent::Handle<ent::Entity>
createPortalEntity(GameContext & aContext,
                   ent::Phase & aInit,
                   const math::Position<2, int> & aPos);
ent::Handle<ent::Entity>
createCopPenEntity(GameContext & aContext,
                   ent::Phase & aInit,
                   const math::Position<2, int> & aPos);
ent::Handle<ent::Entity>
createPlayerSpawnEntity(GameContext & aContext,
                        ent::Phase & aInit,
                        const math::Position<2, int> & aPos);

void 
fillSlotWithPlayer(GameContext & aContext,
                   ent::Phase & aInit,
                   component::ControllerType aControllerType,
                   ent::Handle<ent::Entity> aSlot,
                   int aControllerId = 0);

ent::Handle<ent::Entity> makeText(GameContext & aContext,
                                  ent::Phase & aPhase,
                                  std::string aString,
                                  std::shared_ptr<snac::Font> aFont,
                                  math::hdr::Rgba_f aColor,
                                  math::Position<2, float> aPosition_unitscreen);

ent::Handle<ent::Entity> createMenuItem(GameContext & aContext,
                                        ent::Phase & aInit,
                                        const math::Position<2, int> & aPos);

} // namespace snacgame
} // namespace ad
