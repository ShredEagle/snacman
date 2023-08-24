#pragma once


#include "Camera.h"
#include "Model.h"
#include "Loader.h"
#include "Repositories.h"

#include <graphics/ApplicationGlfw.h>


namespace ad::imguiui {
    class ImguiUi;
} // namespace ad::imguiui


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

struct HardcodedUbos
{
    HardcodedUbos(Storage & aStorage);

    void addUbo(Storage & aStorage, BlockSemantic aSemantic, graphics::UniformBufferObject && aUbo);

    RepositoryUBO mUboRepository;
    graphics::UniformBufferObject * mFrameUbo;
    graphics::UniformBufferObject * mViewingUbo;
    graphics::UniformBufferObject * mModelTransformUbo;
};

struct RenderGraph
{
    RenderGraph(const std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                const std::filesystem::path & aModelFile,
                const imguiui::ImguiUi & aImguiUi);

    // for camera movements, should be moved out to the simuation of course
    void update(float aDeltaTime);
    void render();

    std::shared_ptr<graphics::AppInterface> mGlfwAppInterface;
    Storage mStorage;
    Scene mScene;
    GenericStream mInstanceStream;
    Loader mLoader;
    HardcodedUbos mUbos;

    Camera mCamera;
    
    // TODO #camera allow both control modes, ideally via DearImgui
    OrbitalControl mCameraControl;
    FirstPersonControl mFirstPersonControl;
};


} // namespace ad::renderer