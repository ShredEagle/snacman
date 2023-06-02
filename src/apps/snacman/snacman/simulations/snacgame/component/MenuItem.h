#pragma once

#include "../scene/Scene.h"

#include <unordered_map>

namespace ad {
namespace snacgame {
namespace component {

struct MenuItem
{
    std::string mName;
    bool mSelected;

    std::unordered_map<int, std::string> mNeighbors;
    scene::Transition mTransition;
};

} // namespace component
} // namespace snacgame
} // namespace ad
