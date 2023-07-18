#pragma once

#include "Scene.h"
#include "entity/Query.h"
#include "snac-renderer/Camera.h"
#include "snacman/EntityUtilities.h"

#include <string>

namespace ad {

namespace snac {
struct Time;
}

struct RawInput;

namespace snacgame {

struct GameContext;

namespace component {
struct MappingContext;
struct PlayerSlot;
} // namespace component

namespace scene {

class JoinGameScene : public Scene
{
public:
    JoinGameScene(GameContext & aGameContext,
              ent::Wrap<component::MappingContext> & aMappingContext);

    void update(const snac::Time & aTime, RawInput & aInput) override;

    void onEnter(Transition aTransition) override;

    void onExit(Transition aTransition) override;

    static constexpr char sFromMenuTransition[] = "JoinGameFromMenu";
    static constexpr char sFromGameTransition[] = "FromGame";
    static constexpr char sQuitTransition[] = "QuitToMenu";
    static constexpr char sToGameTransition[] = "ToGame";

private:

    ent::Handle<ent::Entity> mJoinGameRoot;
};

} // namespace scene
} // namespace snacgame
} // namespace ad
