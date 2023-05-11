#include "SettingsScene.h"

namespace ad {
namespace snacgame {
namespace scene {

void SettingsScene::setup(const Transition & aTransition, RawInput & aInput) {}

void SettingsScene::teardown(RawInput & aInput) {}

std::optional<Transition> SettingsScene::update(const snac::Time & aTime,
                                                RawInput & aInput)
{
    return std::nullopt;
}

} // namespace scene
} // namespace snacgame
} // namespace ad
