#pragma once


#include <renderer/GL_Loader.h>

#include <snac-renderer-V2/Model.h>


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
    GLenum mPrimitiveMode = 0;
    GLenum mIndicesType = 0;

    const IntrospectProgram * mProgram;
    const graphics::VertexArrayObject * mVao;
    // TODO I am not sure having the material context here is a good idea, it feels a bit high level
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


PassCache preparePass(StringKey aPass,
                      const PartList & aPartList,
                      Storage & aStorage);


} // namespace ad::renderer