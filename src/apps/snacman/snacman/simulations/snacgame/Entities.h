#pragma once

#include "component/Geometry.h"
#include "component/PlayerSlot.h"
#include "snacman/Input.h"
#include "snacman/simulations/snacgame/scene/MenuScene.h"
#include "snacman/simulations/snacgame/scene/Scene.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>
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

void fillSlotWithPlayer(GameContext & aContext,
                        ent::Phase & aInit,
                        ControllerType aControllerType,
                        ent::Handle<ent::Entity> aSlot,
                        int aControllerId = 0);

ent::Handle<ent::Entity>
makeText(GameContext & aContext,
         ent::Phase & aPhase,
         std::string aString,
         std::shared_ptr<snac::Font> aFont,
         const math::hdr::Rgba_f & aColor,
         const math::Position<2, float> & aPosition_unitscreen,
         const math::Size<2, float> & aScale = {1.f, 1.f});

ent::Handle<ent::Entity>
createMenuItem(GameContext & aContext,
               ent::Phase & aInit,
               std::string aString,
               std::shared_ptr<snac::Font> aFont,
               const math::Position<2, float> & aPos,
               const std::unordered_map<int, std::string> & aNeighbors,
               const scene::Transition & aTransition,
               bool aSelected = false,
               const math::Size<2, float> & aScale = {1.f, 1.f});

bool findSlotAndBind(GameContext & aContext,
                     ent::Phase & aBindPhase,
                     ent::Query<component::PlayerSlot> & aSlots,
                     ControllerType aType,
                     int aIndex);

} // namespace snacgame
} // namespace ad
