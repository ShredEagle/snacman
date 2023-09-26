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


/// @brief A list of parts to be drawn, each associated to a Material and a transformation.
/// It is intended to be reused accross distinct passes inside a frame (or even accross frame for static parts).
struct PartList
{
    // The individual renderer::Objects transforms. Several parts might index to the same transform.
    // (Same way several parts might index the same material parameters)
    std::vector<math::AffineMatrix<4, GLfloat>> mInstanceTransforms;

    // SOA
    std::vector<const Part *> mParts;
    std::vector<const Material *> mMaterials;
    std::vector<GLsizei> mTransformIdx;
};


/// @brief Entry to populate the GL_DRAW_INDIRECT_BUFFER used with indexed (glDrawElements) geometry.
struct DrawElementsIndirectCommand
{
    GLuint  mCount;
    GLuint  mInstanceCount;
    GLuint  mFirstIndex;
    GLuint  mBaseVertex;
    GLuint  mBaseInstance;
};


/// @brief Entry to populate the instance buffer (attribute divisor == 1).
/// Each instance (mapping to a `Part` in client data model) has a pose and a material.
struct DrawInstance
{
    GLsizei mInstanceTransformIdx; // index in the instance UBO
    GLsizei mMaterialIdx;
};


/// @brief Store state and parameters required to issue a GL draw call.
struct DrawCall
{
    GLenum mPrimitiveMode = NULL;
    GLenum mIndicesType = NULL;

    const IntrospectProgram * mProgram;
    const graphics::VertexArrayObject * mVao;

    GLsizei mPartCount;
};


// TODO rename to DrawXxx
struct PassCache
{
    std::vector<DrawCall> mCalls;

    // SOA at the moment, one entry per part
    std::vector<DrawElementsIndirectCommand> mDrawCommands;
    std::vector<DrawInstance> mDrawInstances; // Intended to be loaded as a GL instance buffer
};

struct Scene
{
    Scene & addToRoot(Instance aInstance)
    {
        mRoot.push_back(Node{.mInstance = std::move(aInstance)});
        return *this;
    }

    std::vector<Node> mRoot; 

    PartList populatePartList() const;
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

    void loadDrawBuffers(const PartList & aPartList, const PassCache & aPassCache);

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