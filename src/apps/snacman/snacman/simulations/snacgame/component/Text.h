#pragma once

#include "imguiui/ImguiUi.h"
#include "math/Interpolation/ParameterAnimation.h"
#include "snacman/Timing.h"

#include <implot.h>
#include <math/Color.h>
#include <reflexion/NameValuePair.h>
#include <snacman/serialization/Serial.h>
#include <string>
#include <variant>

namespace ad {
namespace snacgame {
namespace component {

struct Text
{
    renderer::ClientText mString;
    math::hdr::Rgba_f mColor;
    // #complications This member illustrate the case of coupled data:
    // The size is coupled to the ClientText 
    // (a given string rendered with a given font always has the same size)
    // In particular, there is a risk of forgetting to initialize it.
    // It would traditionally be adressed by a constructor guaranteeing post-conditions.
    math::Size<2, float> mDimensions;

    // TODO Ad 2024/11/27: #text This should not be here, but the design lacked so much foresight
    std::shared_ptr<FontAndPart> mFontRef;

    template <class T_witness>
    void describeTo(T_witness && aWitness)
    {
        aWitness.witness(NVP(mString));
        aWitness.witness(NVP(mColor));
        aWitness.witness(NVP(mDimensions));
        aWitness.witness(NVP(mFontRef));
    }
};

REFLEXION_REGISTER(Text)

Text & changeString(Text & aText, const std::string & aNewString, const renderer::Font & aFont);

struct TextZoom
{
    snac::Clock::time_point mStartTime;
    math::ParameterAnimation<float,
                             math::AnimationResult::FullRange,
                             math::None,
                             math::ease::MassSpringDamper>
        mParameter{
            math::ease::MassSpringDamper<float>(
                        1.f,
                        1.2f,
                        1.f
            ),
            1.f
        };

    template <class T_witness>
    void describeTo(T_witness && aWitness)
    {
        if (std::holds_alternative<nlohmann::json *>(aWitness.mData))
        {
            aWitness.witness(NVP(mStartTime));
            aWitness.witness(NVP(mParameter));
        }
        else if (std::holds_alternative<imguiui::ImguiUi *>(aWitness.mData))
        {
            debugRender("TextZoom");
        }
    }

    void debugRender(const char * n);
};

REFLEXION_REGISTER(TextZoom)

} // namespace component
} // namespace snacgame
} // namespace ad
