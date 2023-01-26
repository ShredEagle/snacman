#include "SettingsScene.h"

namespace ad {
namespace snacgame {
namespace scene {

void SettingsScene::setup(const Transition & aTransition) {}

void SettingsScene::teardown() {}

std::optional<Transition> SettingsScene::update(float aDelta,
                                                const RawInput & aInput)
{
    return std::nullopt;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
