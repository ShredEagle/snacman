#include "Render.h"

#include <renderer/Uniforms.h>

namespace ad {
namespace snac {


inline const GLchar* gVertexShader = R"#(
#version 420

layout(location=0) in vec3 ve_VertexPosition_l;
layout(location=1) in vec3 ve_Normal_l;

layout(std140) uniform ViewingBlock
{
    mat4 u_Camera;
    mat4 u_Projection;
};

//uniform vec4 u_Albedo;

// Use diffuse color alpha for surface opacity
out vec4 ex_DiffuseColor;
out vec4 ex_Position_v;
out vec4 ex_Normal_v;

void main(void)
{
    ex_Position_v = u_Camera * vec4(ve_VertexPosition_l, 1.f);
    gl_Position = u_Projection * ex_Position_v;
    ex_Normal_v = u_Camera * vec4(ve_Normal_l, 0.f);
    ex_DiffuseColor = vec4(0.5f, 0.5f, 0.5f, 1.f);
}
)#";


inline const GLchar* gFragmentShader = R"#(
#version 420

in vec4 ex_DiffuseColor;
in vec4 ex_Position_v;
in vec4 ex_Normal_v;

uniform vec3 u_LightPosition_v;
uniform vec3 u_LightColor;

out vec4 out_Color;

void main(void)
{
    // Everything in camera space
    vec3 light = normalize(u_LightPosition_v - ex_Position_v.xyz);
    vec3 view = vec3(0., 0., 1.);
    vec3 h = normalize(view + light);
    vec3 normal = normalize(ex_Normal_v.xyz); // cannot normalize in vertex shader, as interpolation change norm.
    
    vec3 ambientColor = vec3(0.1, 0.2, 0.1);
    vec3 specularColor = ex_DiffuseColor.rgb; // metal like
    float specularExponent = 32;

    vec3 diffuse = 
        ex_DiffuseColor.rgb * (ambientColor + u_LightColor * max(0.f, dot(normal, light)));
    vec3 specular = 
        u_LightColor * specularColor.rgb * pow(max(0.f, dot(normal, h)), specularExponent);
    vec3 color = diffuse + specular;

    // Gamma correction
    float gamma = 2.2;
    out_Color = vec4(pow(color, vec3(1./gamma)), ex_DiffuseColor.a);
}
)#";


Renderer::Renderer() :
    mProgram{graphics::makeLinkedProgram({
        {GL_VERTEX_SHADER, gVertexShader},
        {GL_FRAGMENT_SHADER, gFragmentShader},
    })}
{}


void Renderer::render(const Mesh & aMesh, const Camera & aCamera) const
{
    auto depthTest = graphics::scopeFeature(GL_DEPTH_TEST, true);

    constexpr graphics::BindingIndex viewingLocation{1};
    bind(aCamera.mViewing, viewingLocation);

    GLuint viewingBlockIndex;
    if(viewingBlockIndex = glGetUniformBlockIndex(mProgram, "ViewingBlock") == GL_INVALID_INDEX)
    {
        throw std::logic_error{"Invalid block name."};
    }
    glUniformBlockBinding(mProgram, viewingBlockIndex, viewingLocation);

    // Converted to HDR in order to normalize the values
    setUniform(mProgram, "u_LightColor", to_hdr(math::sdr::gWhite));

    //graphics::ScopedBind boundVAO{aMesh.mStream.mVertexArray};
    bind(aMesh.mStream.mVertexArray);
    {
        const VertexStream::VertexBuffer & view = aMesh.mStream.mVertexBuffers.front();
        //graphics::ScopedBind boundVBO{aMesh.mStream.mVertexBuffers.front()};
        bind(view.mBuffer);
        {
            const graphics::ClientAttribute & attribute =
                aMesh.mStream.mAttributes.at(Semantic::Position).mAttribute;
            glVertexAttribPointer(0, attribute.mDimension, attribute.mDataType, 
                                  GL_FALSE, static_cast<GLsizei>(view.mStride), (void *)attribute.mOffset);
            glEnableVertexAttribArray(0);
        }
        {
            const graphics::ClientAttribute & attribute =
                aMesh.mStream.mAttributes.at(Semantic::Normal).mAttribute;
            glVertexAttribPointer(1, attribute.mDimension, attribute.mDataType, 
                                  GL_FALSE, static_cast<GLsizei>(view.mStride), (void *)attribute.mOffset);
            glEnableVertexAttribArray(1);
        }
    }

    use(mProgram);
    glDrawArrays(GL_TRIANGLES, 0, aMesh.mStream.mVertexCount);
}


} // namespace snac
} // namespace ad
