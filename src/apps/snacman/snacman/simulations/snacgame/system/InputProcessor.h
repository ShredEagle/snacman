#pragma once

#include "entity/Wrap.h"
#include <entity/EntityManager.h>
#include <entity/Query.h>

namespace ad {

struct RawInput;

namespace snacgame {

struct GameContext;
struct ControllerCommand;

namespace component {
struct PlayerSlot;
struct Controller;
struct MappingContext;
} // namespace component

namespace system {

class InputProcessor
{
public:
    InputProcessor(GameContext & aGameContext,
                   ent::Wrap<component::MappingContext> & aMappingContext);

    std::vector<ControllerCommand> mapControllersInput(RawInput & aInput);

private:
    ent::Query<component::PlayerSlot, component::Controller> mPlayers;
    ent::Wrap<component::MappingContext> * mMappingContext;
};

} // namespace system
} // namespace snacgame
} // namespace ad
