#pragma once

#include "snacman/simulations/snacgame/component/PoseScreenSpace.h"
#include "snacman/simulations/snacgame/component/Geometry.h"
#include "snacman/simulations/snacgame/system/MovementIntegration.h"
#include "../GameContext.h"
#include "../component/Text.h"

#include <cstdio>
#include <entity/Query.h>

namespace ad {
namespace snacgame {
namespace system {

class TextZoomSystem
{
public:
    TextZoomSystem(GameContext & aGameContext) : 
        mText(aGameContext.mWorld),
        mGameContext{&aGameContext}
    {}

    void update(const snac::Time & aTime)
    {
        // Compute text position and scale
        mText.each([this](ent::Handle<ent::Entity> aHandle, const component::TextZoom & aTextZoom, component::PoseScreenSpace & aPose, component::Text & aText) {
            float duration = (float)snac::asSeconds(snac::Clock::now() - aTextZoom.mStartTime);

            float value = aTextZoom.mParameter.at(duration);

            aPose.mScale = {value, value};

            math::Size<2, float> size = aText.mDimensions * value;
            const math::Size<2, int> & windowSize =
                mGameContext->mAppInterface.getFramebufferSize();
            aPose.mPosition_u = {0.f - (size.width() / (float)windowSize.width()),
                                     0.f - (size.height() / (float)windowSize.height())};
        });
    }

private:
    ent::Query<component::TextZoom, component::PoseScreenSpace,component::Text> mText;
    GameContext * mGameContext;
};

}
}
}
