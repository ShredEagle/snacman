#pragma once

#include "GameParameters.h"

#include "component/GlobalPose.h"
#include "snacman/simulations/snacgame/component/Geometry.h"
#include "snacman/simulations/snacgame/component/LevelTags.h"
#include "snacman/simulations/snacgame/component/PowerUp.h"

#include <entity/Entity.h>
#include <entity/Query.h>

#include <math/Color.h>
#include <math/Quaternion.h>
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

constexpr float gPillHeight = 6 * gCellSize * 0.1f;
constexpr float gPlayerHeight = 0 * gCellSize * 0.1f;
constexpr float gLevelHeight = 0 * gCellSize * 0.1f;

void addGeoNode(
    ent::Phase & aPhase,
    GameContext & aContext,
    ent::Entity & aEnt,
    math::Position<3, float> aPos = math::Position<3, float>::Zero(),
    float aScale = 1.f,
    math::Size<3, float> aInstanceScale = {1.f, 1.f, 1.f},
    math::Quaternion<float> aOrientation = math::Quaternion<float>::Identity(),
    math::hdr::Rgba_f aColor = math::hdr::gWhite<float>);

void addMeshGeoNode(
    ent::Phase & aPhase,
    GameContext & aContext,
    ent::Entity & aEnt,
    const char * aModelPath,
    math::Position<3, float> aPos = math::Position<3, float>::Zero(),
    float aScale = 1.f,
    math::Size<3, float> aInstanceScale = {1.f, 1.f, 1.f},
    math::Quaternion<float> aOrientation = math::Quaternion<float>::Identity(),
    math::hdr::Rgba_f aColor = math::hdr::gWhite<float>);

ent::Handle<ent::Entity> createWorldText(GameContext & aContext,
                                         std::string aText,
                                         component::GlobalPose aPose);

ent::Handle<ent::Entity> createPill(GameContext & aContext,
                                    ent::Phase & aPhase,
                                    const math::Position<2, float> & Pos);

ent::Handle<ent::Entity> createPowerUp(GameContext & aContext,
                                       ent::Phase & aPhase,
                                       const math::Position<2, float> & Pos);
ent::Handle<ent::Entity> createPlayerPowerUp(GameContext & aContext, const component::PowerUpType aType);

ent::Handle<ent::Entity>
createPathEntity(GameContext & aContext,
                 ent::Phase & aPhase,
                 const math::Position<2, float> & aPos);
ent::Handle<ent::Entity>
createPortalEntity(GameContext & aContext,
                   ent::Phase & aPhase,
                   const math::Position<2, float> & aPos,
                   int aPortalIndex);
void addPortalInfo(component::Portal & aPortal, const component::Geometry & aGeo, math::Vec<3, float> aDirection);
ent::Handle<ent::Entity>
createCopPenEntity(GameContext & aContext,
                   const math::Position<2, float> & aPos);
ent::Handle<ent::Entity>
createPlayerSpawnEntity(GameContext & aContext,
                        ent::Phase & aPhase,
                        const math::Position<2, float> & aPos);

ent::Handle<ent::Entity> fillSlotWithPlayer(GameContext & aContext,
                                            ControllerType aControllerType,
                                            ent::Handle<ent::Entity> aSlot,
                                            int aControllerId = 0);

ent::Handle<ent::Entity> createStageDecor(GameContext & aContext);

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
                ent::Query<component::PlayerSlot> & aSlots,
                ControllerType aType,
                int aIndex);

void swapPlayerPosition(ent::Phase & aPhase, ent::Handle<ent::Entity> aPlayer, ent::Handle<ent::Entity> aOther);
void removeRoundTransientPlayerComponent(ent::Phase & aPhase, ent::Handle<ent::Entity> aHandle);
ent::Handle<ent::Entity> removePlayerFromGame(ent::Phase & aPhase,
                                              ent::Handle<ent::Entity> aHandle);

ent::Handle<ent::Entity> createTargetArrow(GameContext & aContext, const math::hdr::Rgba_f & aColor);

} // namespace snacgame
} // namespace ad
