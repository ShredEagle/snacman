// Splitted between two-files because there was an error on Windows:
// fatal  error C1128: number of sections exceeded object file format limit:
// compile with /bigobj

#include "GameScene.h"
#include "snacman/Input.h"

#include "../GameContext.h"
#include "../Entities.h"
#include "../typedef.h"

#include <snacman/QueryManipulation.h>
#include <snac-renderer/Camera.h>

namespace ad {
namespace snacgame {
namespace scene {

void GameScene::onEnter(Transition aTransition)
{
    EntHandle camera = snac::getFirstHandle(mCameraQuery);
    snac::Orbital & camOrbital = camera.get()->get<snac::Orbital>();
    camOrbital.mSpherical.polar() = gInitialCameraSpherical.polar();
    camOrbital.mSpherical.radius() = gInitialCameraSpherical.radius();
}

} // namespace scene
} // namespace snacgame
} // namespace ad
