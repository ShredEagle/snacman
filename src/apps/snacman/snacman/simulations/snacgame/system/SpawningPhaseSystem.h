#pragma once

#include "snacman/simulations/snacgame/component/PoseScreenSpace.h"
#include "../GameContext.h"
#include "../component/Text.h"

#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class SpawningPhase
{
public:
    SpawningPhase(GameContext & aGameContext) : 
        mText(aGameContext.mWorld)
    {}

    void update(const snac::Time & aTime)
    {
        ent::Phase ouais;
        mText.each([&ouais, &aTime](ent::Handle<ent::Entity> aHandle, const component::TextZoom & aTextZoom, component::PoseScreenSpace & aPose) {
            float duration = (float)snac::asSeconds(aTime.mTimepoint - aTextZoom.mStartTime);

            float value = aTextZoom.mParameter.at(duration);

            aPose.mScale = {value, value};

            if (aTextZoom.mParameter.isCompleted(duration))
            {
                aHandle.get(ouais)->remove<component::TextZoom>();
            }
        });
    }

private:
    ent::Query<component::TextZoom, component::PoseScreenSpace> mText;
};

}
}
}
