
#pragma once


#include <snac-renderer/Render.h>

#include <vector>


namespace ad {
namespace snac {


struct Visual
{
    InstanceStream mInstances;
    Mesh mMesh;
};


using VisualEntities = std::vector<Visual>;


inline void renderEntities(const VisualEntities & aEntities,
                           Renderer & aRenderer,
                           snac::UniformRepository & aUniforms,
                           snac::UniformBlocks & aUniformBlocks,
                           snac::TextureRepository & aTextureRepository,
                           const std::vector<Technique::Annotation> & aTechniqueFilter)
{
    for(auto & [instances, mesh] : aEntities)
    {
        aRenderer.render(mesh,
                         instances,
                         aUniforms,
                         aUniformBlocks,
                         aTextureRepository,
                         std::move(aTechniqueFilter));
    }
}


inline void renderEntities(const VisualEntities & aEntities,
                           Renderer & aRenderer,
                           snac::UniformRepository & aUniforms,
                           snac::UniformBlocks & aUniformBlocks,
                           const std::vector<Technique::Annotation> & aTechniqueFilter)
{
    snac::TextureRepository textureRepo;
    renderEntities(aEntities, aRenderer, aUniforms, aUniformBlocks, textureRepo, aTechniqueFilter);
}


} // namespace snac
} // namespace ad

