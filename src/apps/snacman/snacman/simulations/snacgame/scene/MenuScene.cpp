#include "MenuScene.h"

#include "snacman/simulations/snacgame/InputCommandConverter.h"

#include "../Entities.h"
#include <optional>

namespace ad {
namespace snacgame {
namespace scene {

void MenuScene::setup(const Transition & Transition)
{
    ent::Phase init;
    auto start = createMenuItem(mWorld, init, math::Position<2, int>{0, 0});
    auto quit = createMenuItem(mWorld, init, math::Position<2, int>{0, 0});
    mOwnedEntities.push_back(start);
    mOwnedEntities.push_back(quit);
}

void MenuScene::teardown() {
    ent::Phase destroy;
    for (auto handle : mOwnedEntities)
    {
        handle.get(destroy)->erase();
    }
}

std::optional<Transition> MenuScene::update(float aDelta,
                                            const RawInput & aInput)
{
    int keyboardCommand =
        convertKeyboardInput("menu", aInput.mKeyboard, mContext->mKeyboardMapping);

    if (keyboardCommand & gQuitCommand)
    {
        return Transition{.mTransitionName = gQuitTransitionName};
    }

    if (keyboardCommand & gSelectItem)
    {
        return Transition{.mTransitionName = "start"};
    }

    return std::nullopt;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
