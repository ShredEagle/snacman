#pragma once


#include "Camera.h"
#include "Model.h"
#include "Loader.h"

#include <graphics/ApplicationGlfw.h>


namespace ad::renderer {


using DrawList = std::vector<Instance>;

struct Scene
{
    Scene & addToRoot(Instance aInstance)
    {
        mRoot.push_back(Node{.mInstance = std::move(aInstance)});
        return *this;
    }

    std::vector<Node> mRoot; 

    DrawList populateDrawList() const;
};

struct RenderGraph
{
    RenderGraph(const std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                const std::filesystem::path & aModelFile);

    // for camera movements, should be moved out to the simuation of course
    void update(float aDeltaTime);
    void render();

    std::shared_ptr<graphics::AppInterface> mGlfwAppInterface;
    Storage mStorage;
    Scene mScene;
    GenericStream mInstanceStream;
    Loader mLoader;

    Camera mCamera;
    
    // TODO allow both control modes
    OrbitalControl mCameraControl;
    FirstPersonControl mFirstPersonControl;
};


} // namespace ad::renderer