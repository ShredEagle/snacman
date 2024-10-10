#pragma once


#include "../GraphCommon.h"

#include <snac-renderer-V2/Repositories.h>


namespace ad::renderer {


struct Environment;
struct Storage;
struct ViewerPartList;


enum class DepthMethod
{
    Single,
    Cascaded,
};

// Note: Storage cannot be const, as it might be modified to insert missing VAOs, etc
void passOpaqueDepth(const GraphShared & aGraphShared,
                     const ViewerPartList & aPartList,
                     const RepositoryTexture & aTextureRepository,
                     Storage & aStorage,
                     DepthMethod aMethod);


void passForward(const GraphShared & aGraphShared,
                 const ViewerPartList & aPartList,
                 const RepositoryTexture & aTextureRepository,
                 Storage & aStorage,
                 const GraphControls aControls,
                 bool aEnvironmentMapping);


void passDrawSkybox(const GraphShared & aGraphShared,
                    const Environment & aEnvironment,
                    Storage & aStorage,
                    GLenum aCulledFace,
                    const GraphControls aControls);


void passSkyboxBase(const GraphShared & aGraphShared,
                    const IntrospectProgram & aProgram,
                    const Environment & aEnvironment,
                    Storage & aStorage,
                    GLenum aCulledFace,
                    const GraphControls aControls);


} // namespace ad::renderer