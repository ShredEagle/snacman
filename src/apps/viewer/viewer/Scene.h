#pragma once

#include "Lights.h"

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

    LightsData mLights{
        .mDirectionalCount = 1,
        .mPointCount = 1,
        .mAmbientColor = math::hdr::gWhite<GLfloat> / 3.f,
        .mDirectionalLights = {
            DirectionalLight{
                .mDirection = math::Vec<3, GLfloat>{0.f, 0.2f, 1.f}.normalize(),
                .mDiffuseColor = math::hdr::gWhite<GLfloat> / 2.f,
                .mSpecularColor = math::hdr::gWhite<GLfloat> / 3.f,
            },
        },
        .mPointLights = {
            PointLight{
                .mPosition = {2.f, 0.f, 0.f},
                .mRadius = {
                    .mMin = 1.f,
                    .mMax = 10.f,
                },
                .mDiffuseColor = math::hdr::gYellow<GLfloat>,
            },
        },
    };
};


Scene loadScene(const filesystem::path & aSceneFile,
                const GenericStream & aStream,
                Loader & aLoader, 
                Storage & aStorage);


} // namespace ad::renderer