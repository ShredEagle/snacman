#pragma once


#include <snac-renderer-V2/Cubemap.h>
#include <snac-renderer-V2/Model.h>
#include <snac-renderer-V2/Repositories.h>


namespace ad::renderer {


struct ViewerPassCache;
struct Storage;


constexpr std::size_t gMaxDrawInstances = 2048;


// Note: It is likely that this class will need to be specialized for concrete graph classes.
struct GraphControls
{
    inline static const std::vector<StringKey> gForwardKeys{
        "forward",
        "forward_pbr",
        "forward_phong",
        "forward_debug",
    };

    std::vector<StringKey>::const_iterator mForwardPassKey = gForwardKeys.begin() + 1;

    inline static const std::array<GLenum, 3> gPolygonModes{
        GL_POINT,
        GL_LINE,
        GL_FILL,
    };

    std::array<GLenum, 3>::const_iterator mForwardPolygonMode = gPolygonModes.begin() + 2;
};

struct HardcodedUbos
{
    HardcodedUbos(Storage & aStorage);

    RepositoryUbo mUboRepository;
    graphics::UniformBufferObject * mFrameUbo;
    graphics::UniformBufferObject * mViewingUbo;
    graphics::UniformBufferObject * mModelTransformUbo;

    graphics::UniformBufferObject * mJointMatrixPaletteUbo;
    graphics::UniformBufferObject * mLightsUbo;
    graphics::UniformBufferObject * mLightViewProjectionUbo;
    graphics::UniformBufferObject * mShadowCascadeUbo;
};


struct GraphShared
{
    GraphShared(Loader & aLoader, Storage & aStorage);

    HardcodedUbos mUbos;
    GenericStream mInstanceStream;
    graphics::BufferAny mIndirectBuffer;

    // TODO Ad 2024/10/04: Needing this class smells fishy, it is much too specific
    // Skybox rendering
    Skybox mSkybox;
};


// TODO Ad 2024/10/03: Not sure it needs to be part of the API
void loadDrawBuffers(const GraphShared & aGraphShared,
                     const ViewerPassCache & aPassCache);


} // namespace ad::renderer