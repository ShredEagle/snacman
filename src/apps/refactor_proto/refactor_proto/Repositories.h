#pragma once


#include "Commons.h"

#include <renderer/Texture.h>
#include <renderer/UniformBuffer.h>

#include <map>


namespace ad::renderer {


using RepositoryUBO = std::map<BlockSemantic, graphics::UniformBufferObject>;
using RepositoryTexture = std::map<Semantic, const graphics::Texture *>;


} // namespace ad::renderer