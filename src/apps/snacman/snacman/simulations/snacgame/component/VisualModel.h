#pragma once

#include <snac-renderer-V2/Model.h>

namespace ad {
namespace snacgame {
namespace component {


struct VisualModel
{
    renderer::Handle<const renderer::Object> mModel;
    bool mDisableInterpolation = false;
};


} // namespace component
} // namespace cubes
} // namespace ad

