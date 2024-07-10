#pragma once

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

#include <snac-renderer-V2/Model.h>

namespace ad {
namespace snacgame {
namespace component {


struct VisualModel
{
    renderer::Handle<const renderer::Object> mModel;
    bool mDisableInterpolation = false;

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mModel);
        archive & SERIAL_PARAM(mDisableInterpolation);
    }
};

SNAC_SERIAL_REGISTER(VisualModel)


} // namespace component
} // namespace cubes
} // namespace ad

