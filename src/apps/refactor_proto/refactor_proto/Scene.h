#pragma once


#include "Model.h"

#include <math/Matrix.h>

#include <vector>


namespace ad::renderer {


struct PartList;


struct Scene
{
    Scene & addToRoot(Node aNode)
    {
        mRoot.mAabb.uniteAssign(aNode.mAabb);
        mRoot.mChildren.push_back(std::move(aNode));
        return *this;
    }

    Scene & addToRoot(Instance aInstance)
    {
        assert(aInstance.mObject);

        Node node{
            .mInstance = std::move(aInstance),
            .mAabb = aInstance.mObject->mAabb,
        };

        return addToRoot(std::move(node));
    }

    /// @brief Loads a scene defined as data by a sew file.
    /// @param aSceneFile A sew file (json) describing the scene to load.
    Scene LoadFile(const filesystem::path aSceneFile);

    PartList populatePartList() const;

    Node mRoot{
        .mInstance = {
            .mName = "scene-root",
        },
    }; 
};


} // namespace ad::renderer