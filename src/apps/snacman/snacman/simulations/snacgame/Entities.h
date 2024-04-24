#pragma once

#include "GameParameters.h"
#include "Handle_V2.h"
#include "component/PowerUp.h"

#include <snacman/Timing.h>

#include <entity/Entity.h>
#include <entity/Query.h>
#include <math/Color.h>
#include <math/Quaternion.h>
#include <math/Vector.h>

#include <snac-renderer-V2/Model.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace ad {

enum class ControllerType;

// Forward
namespace snac {
struct Font;
struct Model;
} // namespace snac

namespace snacgame {

namespace component {
struct PlayerRoundData;
struct PlayerSlot;
struct Portal;
struct Geometry;
struct GlobalPose;
} // namespace component
namespace scene {
struct Transition;
}

struct GameContext;

//Scene graph component utils
void addGeoNode(
    GameContext & aContext,
    ent::Entity & aEnt,
    math::Position<3, float> aPos = math::Position<3, float>::Zero(),
    float aScale = 1.f,
    math::Size<3, float> aInstanceScale = {1.f, 1.f, 1.f},
    math::Quaternion<float> aOrientation = math::Quaternion<float>::Identity(),
    math::hdr::Rgba_f aColor = math::hdr::gWhite<float>);

renderer::Handle<const renderer::Object> addMeshGeoNode(
    GameContext & aContext,
    ent::Entity & aEnt,
    const char * aModelPath,
    const char * aEffectPath,
    math::Position<3, float> aPos = math::Position<3, float>::Zero(),
    float aScale = 1.f,
    math::Size<3, float> aInstanceScale = {1.f, 1.f, 1.f},
    math::Quaternion<float> aOrientation = math::Quaternion<float>::Identity(),
    math::hdr::Rgba_f aColor = math::hdr::gWhite<float>);

// Level utils
ent::Handle<ent::Entity> createPill(GameContext & aContext,
                                    ent::Phase & aPhase,
                                    const math::Position<2, float> & Pos);
ent::Handle<ent::Entity>
createPathEntity(GameContext & aContext,
                 ent::Phase & aPhase,
                 const math::Position<2, float> & aPos);
ent::Handle<ent::Entity>
createCopPenEntity(GameContext & aContext,
                   const math::Position<2, float> & aPos);
ent::Handle<ent::Entity>
createPlayerSpawnEntity(GameContext & aContext,
                        ent::Phase & aPhase,
                        const math::Position<2, float> & aPos);
ent::Handle<ent::Entity>
createPortalEntity(GameContext & aContext,
                   ent::Phase & aPhase,
                   const math::Position<2, float> & aPos,
                   int aPortalIndex);
void addPortalInfo(GameContext & aContext,
                   ent::Handle<ent::Entity> aLevel,
                   component::Portal & aPortal,
                   const component::Geometry & aGeo,
                   math::Vec<3, float> aDirection);
ent::Handle<ent::Entity> createStageDecor(GameContext & aContext);

// Text utils
ent::Handle<ent::Entity>
makeText(GameContext & aContext,
         ent::Phase & aPhase,
         const std::string & aString,
         std::shared_ptr<snac::Font> aFont,
         const math::hdr::Rgba_f & aColor,
         const math::Position<2, float> & aPosition_unitscreen,
         const math::Size<2, float> & aScale = {1.f, 1.f});

// Menu utils
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

// Player utils
ent::Handle<ent::Entity> addPlayer(GameContext & aContext, const int aControllerIndex);
ent::Handle<ent::Entity> createCrown(GameContext & aContext);
ent::Handle<ent::Entity> createInGamePlayer(GameContext & aContext,
                                            ent::Handle<ent::Entity> aSlotHandle,
                                            const math::Position<3, float> & aPosition);
ent::Handle<ent::Entity> createJoinGamePlayer(GameContext & aContext,
                                            ent::Handle<ent::Entity> aSlotHandle, int aSlotIndex);
void preparePlayerForGame(GameContext & aContext, ent::Handle<ent::Entity> aSlotHandle);
void addBillpadHud(GameContext & aContext, ent::Handle<ent::Entity> aSlotHandle);
void swapPlayerPosition(ent::Phase & aPhase,
                        ent::Handle<ent::Entity> aPlayer,
                        ent::Handle<ent::Entity> aOther);

ent::Handle<ent::Entity> createTargetArrow(GameContext & aContext,
                                           const math::hdr::Rgba_f & aColor);
ent::Handle<ent::Entity>
createHudBillpad(GameContext & aContext,
                 const int aPlayerIndex);



// Powerup utils
ent::Handle<ent::Entity> createPowerUp(GameContext & aContext,
                                       ent::Phase & aPhase,
                                       const math::Position<2, float> & Pos,
                                       const component::PowerUpType aType,
                                       float aSwapPeriod);
ent::Handle<ent::Entity>
createPlayerPowerUp(GameContext & aContext, const component::PowerUpType aType);

ent::Handle<ent::Entity>
createExplosion(GameContext & aContext,
                const math::Position<3, float> & aPosition,
                const snac::Time & aTime);

// Portal utils
ent::Handle<ent::Entity>
createPortalImage(GameContext & aContext,
                  component::PlayerRoundData & aPlayerRoundData,
                  const component::Portal & aPortal);


// Test entity utils
ent::Handle<ent::Entity> createWorldText(GameContext & aContext,
                                         std::string aText,
                                         const component::GlobalPose & aPose);

} // namespace snacgame
} // namespace ad
