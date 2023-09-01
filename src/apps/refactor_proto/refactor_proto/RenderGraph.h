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


struct DrawElementsIndirectCommand
{
    GLuint  mCount;
    GLuint  mInstanceCount;
    GLuint  mFirstIndex;
    GLuint  mBaseVertex;
    GLuint  mBaseInstance;
};


struct DrawInstance
{
    GLsizei mInstanceTransformIdx; // index in the instance UBO
    GLsizei mMaterialIdx;
};


// TODO rename to PartList (also the functions)
struct DrawList
{
    // The individual renderer::Objects transforms. Several parts might index to the same transform.
    // (Same way several parts might index the same material parameters)
    std::vector<math::AffineMatrix<4, GLfloat>> mInstanceTransforms;

    // SOA
    std::vector<const Part *> mParts;
    std::vector<const Material *> mMaterials;
    std::vector<GLsizei> mTransformIdx;
};


// TODO rename to DrawXxx
struct PassCache
{
    GLenum mPrimitiveMode = NULL;
    GLenum mIndicesType = NULL;

    // SOA at the moment
    std::vector<DrawElementsIndirectCommand> mDrawCommands;
    std::vector<DrawInstance> mDrawInstances; // Intended to be loaded as a GL instance buffer
    // TODO program should be the first consolidation dimension
    std::vector<const IntrospectProgram *> mPrograms;
    std::vector<const graphics::VertexArrayObject *> mVaos;
};


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

    void loadDrawBuffers(const DrawList & aDrawList, const PassCache & aPassCache);

    std::shared_ptr<graphics::AppInterface> mGlfwAppInterface;
    Storage mStorage;
    Scene mScene;
    GenericStream mInstanceStream;
    Loader mLoader;
    HardcodedUbos mUbos;
    graphics::BufferAny mIndirectBuffer;

    Camera mCamera;
    
    // TODO #camera allow both control modes, ideally via DearImgui
    OrbitalControl mCameraControl;
    FirstPersonControl mFirstPersonControl;
};


} // namespace ad::renderer