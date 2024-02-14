#pragma once

#include <snac-renderer-V2/Model.h>

namespace ad {
namespace snacgame {
namespace component {


struct VisualModel
{
    std::shared_ptr<renderer::Node> mModel;
    bool mDisableInterpolation = false;
};


} // namespace component
} // namespace cubes
} // namespace ad

