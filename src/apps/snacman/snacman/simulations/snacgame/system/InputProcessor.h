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
struct PlayerRoundData;
struct Controller;
struct MappingContext;
} // namespace component

namespace system {

class InputProcessor
{
public:
    InputProcessor(GameContext & aGameContext,
                   ent::Wrap<component::MappingContext> & aMappingContext);

    std::vector<ControllerCommand> mapControllersInput(RawInput & aInput, const char * aBoundMode, const char * aUnboundMode);

private:
    ent::Query<component::Controller> mPlayers;
    ent::Wrap<component::MappingContext> * mMappingContext;
};

} // namespace system
} // namespace snacgame
} // namespace ad
