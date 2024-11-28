#pragma once

#include <snac-renderer-V2/Model.h>
#include <snac-renderer-V2/Lights.h>
#include <snac-renderer-V2/Semantics.h>
#include <snac-renderer-V2/graph/text/TextGlsl.h>


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


math::Box<GLfloat> getAabbInParent(const Node & aNode);


// Note: this is not a good design, just a quick prototype to show text in viewer
struct ProtoTexts
{
    struct StringEntities
    {
        // The glyphs in this string
        ClientText mStringGlyphs;
        // The different "instances" of this string
        std::vector<StringEntity_glsl> mEntities;
    };

    TextPart mGlyphPart; // At the moment, the part is pretty much coupled to the Font
    std::vector<StringEntities> mStrings;
};


/// @note The Scene does not contain the camera: this way the same logical scene 
/// can be rendered from multiple views.
struct Scene
{
    Scene & addToRoot(Node aNode)
    {
        mRoot.mAabb.uniteAssign(getAabbInParent(aNode));
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

    LightsDataCommon getLightsInCamera(const Camera & aCamera,
                                     bool aTransformDirectionalLights) const;

    Node mRoot{
        .mInstance = {
            .mName = "scene-root",
        },
    }; 

    ProtoTexts mScreenText; 
    
    LightsDataUi mLights_world{
        LightsDataCommon{
            .mDirectionalCount = 1,
            .mPointCount = 0,
            .mAmbientColor = math::hdr::gWhite<GLfloat> / 12.f,
            .mDirectionalLights = {
                DirectionalLight{
                    .mDirection = math::UnitVec<3, GLfloat>{math::Vec<3, GLfloat>{0.f, -0.2f, -1.f}},
                    .mDiffuseColor = math::hdr::gWhite<GLfloat>,
                    .mSpecularColor = math::hdr::gWhite<GLfloat> / 2.f,
                },
                DirectionalLight{
                    .mDirection = math::UnitVec<3, GLfloat>{math::Vec<3, GLfloat>{0.77f, -0.4f, -0.5f}},
                    .mDiffuseColor = math::hdr::Rgb_f{1.f, 0.45f, 0.f},
                    .mSpecularColor = math::hdr::Rgb_f{1.f, 0.45f, 0.f},
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
        },
        LightsDataToggleShadow{
            .mDirectionalLightProjectShadow = []{
                auto result = make_filled_array<ProjectShadowToggle, gMaxLights>(ProjectShadowToggle{});
                result[0] = result[1] = ProjectShadowToggle{.mIsProjectingShadow = true};
                return result;
            }()
        }
    };

    std::optional<Environment> mEnvironment;
};


Scene loadScene(const filesystem::path & aSceneFile,
                const GenericStream & aStream,
                Loader & aLoader, 
                Storage & aStorage);


} // namespace ad::renderer