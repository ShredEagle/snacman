#pragma once


#include "Model.h"

#include <math/Matrix.h>

#include <vector>


namespace ad::renderer {


/// @brief A list of parts to be drawn, each associated to a Material and a transformation.
/// It is intended to be reused accross distinct passes inside a frame (or even accross frame for static parts).
struct PartList
{
    // The individual renderer::Objects transforms. Several parts might index to the same transform.
    // (Same way several parts might index the same material parameters)
    std::vector<math::AffineMatrix<4, GLfloat>> mInstanceTransforms;

    // SOA
    std::vector<const Part *> mParts;
    std::vector<const Material *> mMaterials;
    std::vector<GLsizei> mTransformIdx;
};


struct Scene
{
    Scene & addToRoot(Node aNode)
    {
        mRoot.push_back(std::move(aNode));
        return *this;
    }

    Scene & addToRoot(Instance aInstance)
    {
        mRoot.push_back(Node{.mInstance = std::move(aInstance)});
        return *this;
    }

    /// @brief Loads a scene defined as data by a sew file.
    /// @param aSceneFile A sew file (json) describing the scene to load.
    Scene LoadFile(const filesystem::path aSceneFile);

    std::vector<Node> mRoot; 

    PartList populatePartList() const;
};


void presentTree(const Scene & aScene);


} // namespace ad::renderer