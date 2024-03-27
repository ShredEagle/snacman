#pragma once

#include <snac-renderer-V2/Model.h>

namespace ad {
namespace snacgame {
namespace component {


struct VisualModel
{
    // TODO Ad 2024/03/26: #RV2 API this should probably be a renderer::Object, not a Node
    std::shared_ptr<renderer::Node> mModel;
    bool mDisableInterpolation = false;
};


} // namespace component
} // namespace cubes
} // namespace ad

