#pragma once


#include "LoadInterface.h"
#include "Mesh.h"

#include <platform/Filesystem.h>

#include <resource/ResourceFinder.h>

#include <memory>


namespace ad {
namespace snac {


bool attemptRecompile(Technique & aTechnique);

IntrospectProgram loadProgram(const filesystem::path & aProgram);
Technique loadTechnique(filesystem::path aProgram);


struct TechniqueLoader : public Load<Technique>
{
    /*implicit*/ TechniqueLoader(const resource::ResourceFinder & aFinder) :
        mFinder{aFinder}
    {}
    
    Technique get(const filesystem::path & aProgram) override
    { return loadTechnique(mFinder.pathFor(aProgram)); }

    const resource::ResourceFinder & mFinder;
};


/// \brief Make an Effect with a single Technique, without annotations.
std::shared_ptr<Effect> loadTrivialEffect(Technique aTechnique);

inline std::shared_ptr<Effect> loadTrivialEffect(filesystem::path aProgram)
{ return loadTrivialEffect(loadTechnique(std::move(aProgram))); }


std::shared_ptr<Effect> loadEffect(filesystem::path aEffectFile,
                                   Load<Technique> & aTechniqueAccess);

inline
std::shared_ptr<Effect> loadEffect(filesystem::path aEffectFile,
                                   Load<Technique> && aTechniqueAccess)
{ return loadEffect(aEffectFile, aTechniqueAccess); }

Mesh loadCube(std::shared_ptr<Effect> aEffect, std::string_view aName = "procedural_cube");

Model loadModel(filesystem::path aGltf, std::shared_ptr<Effect> aEffect);

} // namespace graphics
} // namespace ad