#pragma once

#include "Lights.h"

#include <snac-renderer-V2/Model.h>
#include <snac-renderer-V2/Semantics.h>


namespace ad::renderer {

class Camera;
struct Loader;
struct ViewerPartList;


struct Environment
{
    enum Type
    {
        Cubemap,
        Equirectangular,
    };

    Handle<const graphics::Texture> getEnvMap() const
    { return mTextureRepository.at(semantic::gEnvironmentTexture); }

    Handle<const graphics::Texture> getFilteredRadiance() const
    { return mTextureRepository.at(semantic::gFilteredRadianceEnvironmentTexture); }

    Handle<const graphics::Texture> getFilteredIrradiance() const
    { return mTextureRepository.at(semantic::gFilteredIrradianceEnvironmentTexture); }

    Type mType;
    RepositoryTexture mTextureRepository;
    std::filesystem::path mMapFile; // notably used when dumping the cubemap, to derive a destination
};


inline std::string to_string(Environment::Type aEnvironment)
{
    switch(aEnvironment)
    {
        case Environment::Cubemap:
            return "cubemap";
        case Environment::Equirectangular:
            return "equirectangular";
        default:
            throw std::invalid_argument{"Environment type value is not known."};
    }
}


/// @note The Scene does not contain the camera: this way the same logical scene 
/// can be rendered from multiple views.
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

    LightsData getLightsInCamera(const Camera & aCamera,
                                 bool aTransformDirectionalLights) const;

    Node mRoot{
        .mInstance = {
            .mName = "scene-root",
        },
    }; 

    
    LightsData mLights_world{
        .mDirectionalCount = 0,
        .mPointCount = 0,
        .mAmbientColor = math::hdr::gWhite<GLfloat> / 12.f,
        .mDirectionalLights = {
            DirectionalLight{
                .mDirection = math::UnitVec<3, GLfloat>{math::Vec<3, GLfloat>{0.f, -0.2f, -1.f}},
                .mDiffuseColor = math::hdr::gWhite<GLfloat>,
                .mSpecularColor = math::hdr::gWhite<GLfloat> / 2.f,
            },
        },
        .mPointLights = {
            PointLight{
                .mPosition = {2.f, 0.f, 0.f},
                .mRadius = {
                    .mMin = 1.f,
                    .mMax = 10.f,
                },
                .mDiffuseColor = math::hdr::gGreen<GLfloat>,
            },
        },
    };

    std::optional<Environment> mEnvironment;
};


Scene loadScene(const filesystem::path & aSceneFile,
                const GenericStream & aStream,
                Loader & aLoader, 
                Storage & aStorage);


} // namespace ad::renderer