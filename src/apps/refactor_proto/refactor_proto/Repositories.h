#pragma once


#include "Commons.h"

#include <renderer/Texture.h>
#include <renderer/UniformBuffer.h>

#include <map>


namespace ad::renderer {


// TODO the UBO should also be stored in some Storage instance, not an ad-hoc repo
using RepositoryUBO = std::map<BlockSemantic, const graphics::UniformBufferObject *>;
using RepositoryTexture = std::map<Semantic, const graphics::Texture *>;


} // namespace ad::renderer