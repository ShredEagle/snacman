#pragma once


#include "Mesh.h"

#include <platform/Filesystem.h>

#include <resource/ResourceFinder.h>

#include <memory>


namespace ad {
namespace snac {


bool attemptRecompile(Technique & aTechnique);

IntrospectProgram loadProgram(const filesystem::path & aProgram);
Technique loadTechnique(filesystem::path aProgram);

/// \deprecated An effect should not be made from a single program file.
std::shared_ptr<Effect> loadTrivialEffect(filesystem::path aProgram);

/// \deprecated Should not take a finder directly, but a resource manager
std::shared_ptr<Effect> loadEffect(filesystem::path aEffectFile,
                                   const resource::ResourceFinder & aFinder);

Mesh loadCube(std::shared_ptr<Effect> aEffect);

Mesh loadModel(filesystem::path aGltf, std::shared_ptr<Effect> aEffect);


} // namespace graphics
} // namespace ad