#pragma once

#include <snac-renderer-V2/Model.h>

#include <memory>

namespace ad::snacgame {


template <class T_resource>
using Handle = std::shared_ptr<T_resource>;


} // namespace ad::snacgame