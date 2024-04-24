#pragma once


#include <snac-renderer-V2/Pass.h>


// Note: This file with everithing prefixed by Viewer is a good argument to change
// the namespace of this application code to "viewer"

namespace ad::renderer {


struct ViewerPartList : public PartList
{
    // The individual renderer::Objects transforms. Several parts might index to the same transform.
    // (Same way several parts might index the same material parameters)
    std::vector<math::AffineMatrix<4, GLfloat>> mInstanceTransforms;

    // All the palettes are concatenated in a single unidimensionnal container.
    std::vector<Rig::Pose> mRiggingPalettes;

    // SOA, continued
    std::vector<GLsizei> mTransformIdx;
    std::vector<GLsizei> mPaletteOffset; // the first bone index for a each part in the global matrix palette
};


/// @brief Entry to populate the instance buffer (attribute divisor == 1).
/// Each instance (mapping to a `Part` in client data model) has a pose and a material.
// TODO Ad 2024/03/20: Why does it duplicate Loader.h InstanceData? (modulo the alias types)
// TODO move into the viewer
struct ViewerDrawInstance
{
    GLsizei mInstanceTransformIdx; // index in the instance UBO
    GLsizei mMaterialIdx;
    GLsizei mMatrixPaletteOffset;
};


struct ViewerPassCache : public PassCache
{
    std::vector<ViewerDrawInstance> mDrawInstances; // Intended to be loaded as a GL instance buffer
};

/// @brief From a PartList, generates the PassCache for a given pass.
/// @param aPass Pass name.
/// @param aPartList The PartList that should be rendered.
ViewerPassCache prepareViewerPass(StringKey aPass,
                                  const ViewerPartList & aPartList,
                                  Storage & aStorage);



} // namespace ad::renderer