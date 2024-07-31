#pragma once


#include "Commons.h"
#include "Handle.h"

#include <renderer/Texture.h>
#include <renderer/UniformBuffer.h>

#include <map>


namespace ad::renderer {


// TODO the UBO should also be stored in some Storage instance, not an ad-hoc repo
using RepositoryUbo = std::map<BlockSemantic, Handle<const graphics::UniformBufferObject>>;
using RepositoryTexture = std::map<Semantic, Handle<const graphics::Texture>>;


} // namespace ad::renderer