#pragma once


#include "../GraphCommon.h"

#include <snac-renderer-V2/Repositories.h>
#include <snac-renderer-V2/graph/DepthMethod.h>


namespace ad::renderer {


struct Environment;
struct Storage;
struct ViewerPartList;


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
                 // TODO Ad 2024/10/16: #pass_API The multiplication of ad-hoc controls on the passes interface
                 // seems indicative that the pass abstraction is not well defined.
                 bool aCascadedShadowMap,
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