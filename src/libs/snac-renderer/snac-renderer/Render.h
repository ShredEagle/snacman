#pragma once

#include "Camera.h"
#include "Mesh.h"
#include "SetupDrawing.h"
#include "UniformParameters.h"

#include <renderer/GL_Loader.h>


namespace ad {
namespace snac {


inline void clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}


const IntrospectProgram * findTechnique(
    const Material & aMaterial,
    const std::vector<Technique::Annotation> & aTechniqueFilter);


class Renderer
{
public:
    /// \deprecated
    void render(const Mesh & aMesh,
                const InstanceStream & aInstances,
                UniformRepository & aUniforms,
                UniformBlocks & aUniformBlocks,
                TextureRepository & aTextures,
                const std::vector<Technique::Annotation> & aTechniqueFilter = {});

    /// \deprecated
    void render(const Mesh & aMesh,
                const InstanceStream & aInstances,
                UniformRepository & aUniforms,
                UniformBlocks & aUniformBlocks,
                const std::vector<Technique::Annotation> & aTechniqueFilter = {})
    { 
        TextureRepository textureRepo;
        render(aMesh, aInstances, aUniforms, aUniformBlocks, textureRepo, aTechniqueFilter);
    }

private:
    VertexArrayRepository mVertexArrayRepo;
    WarningRepository mWarningRepo;
};


} // namespace snac
} // namespace ad