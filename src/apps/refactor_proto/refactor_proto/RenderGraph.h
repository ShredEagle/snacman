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

    void render();

    std::shared_ptr<graphics::AppInterface> mGlfwAppInterface;
    Storage mStorage;
    Scene mScene;
    SemanticBufferViews mInstanceStream;
    Loader mLoader;

    Camera mCamera;
    OrbitalControl mCameraControl;
};


} // namespace ad::renderer