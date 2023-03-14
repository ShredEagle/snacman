
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
                           const snac::UniformRepository & aUniforms,
                           const snac::UniformBlocks & aUniformBlocks,
                           const snac::TextureRepository & aTextureRepository,
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



} // namespace snac
} // namespace ad

