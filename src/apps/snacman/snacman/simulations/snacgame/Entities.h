#pragma once

#include <entity/Entity.h>
#include <entity/Query.h>
#include <math/Color.h>
#include <math/Vector.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace ad {

enum class ControllerType;

// Forward
namespace snac {
struct Font;
}

namespace snacgame {

namespace component {
struct PlayerSlot;
}
namespace scene {
struct Transition;
}

struct GameContext;

ent::Handle<ent::Entity> createPill(GameContext & aContext,
                                    const math::Position<2, float> & Pos);
ent::Handle<ent::Entity>
createPathEntity(GameContext & aContext, const math::Position<2, float> & aPos);
ent::Handle<ent::Entity>
createPortalEntity(GameContext & aContext,
                   const math::Position<2, float> & aPos,
                   int aPortalIndex);
ent::Handle<ent::Entity>
createCopPenEntity(GameContext & aContext,
                   const math::Position<2, float> & aPos);
ent::Handle<ent::Entity>
createPlayerSpawnEntity(GameContext & aContext,
                        const math::Position<2, float> & aPos);

ent::Handle<ent::Entity> fillSlotWithPlayer(GameContext & aContext,
                                            ControllerType aControllerType,
                                            ent::Handle<ent::Entity> aSlot,
                                            int aControllerId = 0);

ent::Handle<ent::Entity>
makeText(GameContext & aContext,
         ent::Phase & aPhase,
         const std::string & aString,
         std::shared_ptr<snac::Font> aFont,
         const math::hdr::Rgba_f & aColor,
         const math::Position<2, float> & aPosition_unitscreen,
         const math::Size<2, float> & aScale = {1.f, 1.f});

ent::Handle<ent::Entity>
createMenuItem(GameContext & aContext,
               ent::Phase & aInit,
               const std::string & aString,
               std::shared_ptr<snac::Font> aFont,
               const math::Position<2, float> & aPos,
               const std::unordered_map<int, std::string> & aNeighbors,
               const scene::Transition & aTransition,
               bool aSelected = false,
               const math::Size<2, float> & aScale = {1.f, 1.f});

std::optional<ent::Handle<ent::Entity>>
findSlotAndBind(GameContext & aContext,
                ent::Phase & aBindPhase,
                ent::Query<component::PlayerSlot> & aSlots,
                ControllerType aType,
                int aIndex);

} // namespace snacgame
} // namespace ad
