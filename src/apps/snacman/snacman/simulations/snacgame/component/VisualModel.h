#pragma once

#include <snac-renderer-V1/Mesh.h>

namespace ad {
namespace snacgame {
namespace component {


struct VisualModel
{
    std::shared_ptr<snac::Model> mModel;
    bool mDisableInterpolation = false;
};


} // namespace component
} // namespace cubes
} // namespace ad

