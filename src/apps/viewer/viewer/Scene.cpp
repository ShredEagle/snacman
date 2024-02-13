#include "Scene.h"

#include "Json.h"
#include "Logging.h"

// TODO Ad 2023/10/06: Move the Json <-> Math type converters to a better place
#include <arte/detail/GltfJson.h>

#include <math/Transformations.h>

#include <snac-renderer-V2/files/Loader.h>

#include <fstream>


namespace ad::renderer {


Scene loadScene(const filesystem::path & aSceneFile,
                const GenericStream & aStream,
                Loader & aLoader, 
                Storage & aStorage)
{
    Scene scene;

    //filesystem::path scenePath = mFinder.pathFor(aSceneFile);
    filesystem::path scenePath = aSceneFile;
    Json description = Json::parse(std::ifstream{scenePath});
    filesystem::path workingDir = scenePath.parent_path();

    for (const auto & entry : description.at("entries"))
    {
        std::vector<std::string> defines = entry["features"].get<std::vector<std::string>>();
        filesystem::path modelFile = entry.at("model");

        Node model = loadBinary(workingDir / modelFile,
                                aStorage,
                                aLoader.loadEffect(entry["effect"], aStorage, defines),
                                aStream);

        // TODO store and load names in the binary file format
        SELOG(info)("Loaded model '{}' with bouding box {}.",
                     modelFile.stem().string(), fmt::streamed(model.mAabb));

        math::Box<GLfloat> poserAabb = model.mAabb;

        Pose pose;
        if(entry.contains("pose"))
        {
            pose.mPosition = entry.at("pose").value<math::Vec<3, float>>("position", {});
            pose.mUniformScale = entry.at("pose").value("uniform_scale", 1.f);

            poserAabb *= math::trans3d::scaleUniform(pose.mUniformScale)
                        * math::trans3d::translate(pose.mPosition);
        }

        Node modelPoser{
            .mInstance = Instance{
                .mPose = pose,
                .mName = entry.at("name").get<std::string>(),
            },
            .mChildren = {std::move(model)},
            .mAabb = poserAabb,
        };
        scene.addToRoot(std::move(modelPoser));
    }

    return scene;
}



} // namespace ad::renderer