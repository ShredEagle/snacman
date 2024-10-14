#pragma once

#include "snacman/serialization/Witness.h"

#include <snac-reflexion/Reflexion.h>
#include <snac-reflexion/Reflexion_impl.h>
#include <reflexion/NameValuePair.h>

#include <snac-renderer-V2/Model.h>

namespace ad {
namespace snacgame {
namespace component {


struct VisualModel
{
    renderer::Handle<const renderer::Object> mModel = nullptr;
    bool mDisableInterpolation = false;

    template<class T_witness>
    void describeTo(T_witness && aWitness)
    {
        //TODO(franz): add serialization of model
        aWitness.witness(NVP(mModel));
        aWitness.witness(NVP(mDisableInterpolation));
    }
};

REFLEXION_REGISTER(VisualModel)

} // namespace component
} // namespace cubes
} // namespace ad

