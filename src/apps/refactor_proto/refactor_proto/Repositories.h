#pragma once


#include "Commons.h"

#include <renderer/UniformBuffer.h>

#include <map>


namespace ad::renderer {


using RepositoryUBO = std::map<BlockSemantic, graphics::UniformBufferObject>;


} // namespace ad::renderer