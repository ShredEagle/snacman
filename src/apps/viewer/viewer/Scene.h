#pragma once


#include <snac-renderer-V2/Model.h>


namespace ad::renderer {

struct Loader;
struct ViewerPartList;


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

    ViewerPartList populatePartList() const;

    Node mRoot{
        .mInstance = {
            .mName = "scene-root",
        },
    }; 
};


Scene loadScene(const filesystem::path & aSceneFile,
                const GenericStream & aStream,
                Loader & aLoader, 
                Storage & aStorage);


} // namespace ad::renderer