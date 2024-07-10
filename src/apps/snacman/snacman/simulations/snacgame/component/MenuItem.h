#pragma once

#include "../scene/Scene.h"

#include <snacman/detail/Reflexion.h>
#include <snacman/detail/Reflexion_impl.h>
#include <snacman/detail/Serialization.h>

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

    template <SnacArchive T_archive>
    void serialize(T_archive & archive)
    {
        archive & SERIAL_PARAM(mName);
        archive & SERIAL_PARAM(mSelected);
        archive & SERIAL_PARAM(mNeighbors);
        archive & SERIAL_PARAM(mTransition);
    }
};

SNAC_SERIAL_REGISTER(MenuItem)

} // namespace component
} // namespace snacgame
} // namespace ad
