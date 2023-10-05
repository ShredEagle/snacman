#pragma once


#include "Camera.h"
#include "Model.h"
#include "Loader.h"
#include "Repositories.h"
#include "Scene.h"

#include <graphics/ApplicationGlfw.h>


namespace ad::imguiui {
    class ImguiUi;
} // namespace ad::imguiui


namespace ad::renderer {


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
    // TODO I am not sure having the call context here is a good idea, it feels a bit high level
    Handle<MaterialContext> mCallContext;

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

struct HardcodedUbos
{
    HardcodedUbos(Storage & aStorage);

    void addUbo(Storage & aStorage, BlockSemantic aSemantic, graphics::UniformBufferObject && aUbo);

    RepositoryUbo mUboRepository;
    graphics::UniformBufferObject * mFrameUbo;
    graphics::UniformBufferObject * mViewingUbo;
    graphics::UniformBufferObject * mModelTransformUbo;
};

struct RenderGraph
{
    RenderGraph(const std::shared_ptr<graphics::AppInterface> aGlfwAppInterface,
                const std::filesystem::path & aSceneFile,
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