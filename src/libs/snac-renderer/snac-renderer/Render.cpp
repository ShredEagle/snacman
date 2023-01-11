#include "Render.h"

#include "Logging.h"

#include <renderer/BufferUtilities.h>
#include <renderer/Uniforms.h>

namespace ad {
namespace snac {


inline const GLchar* gVertexShader = R"#(
#version 420

layout(location=0) in vec3 ve_Position_l;
layout(location=1) in vec3 ve_Normal_l;

layout(location=4) in mat4 in_LocalToWorld;
// Will be required to support non-uniform scaling.
//layout(location=8) in mat4 in_LocalToWorldInverseTranspose;
layout(location=12) in vec4 in_Albedo;

layout(std140) uniform ViewingBlock
{
    mat4 u_WorldToCamera;
    mat4 u_Projection;
};

out vec4 ex_Position_v;
out vec4 ex_Normal_v;
out vec3 ex_DiffuseColor;
out vec3 ex_SpecularColor;
out float ex_Opacity;

void main(void)
{
    // TODO maybe should be pre-multiplied in client code?
    mat4 localToCamera = u_WorldToCamera * in_LocalToWorld;
    ex_Position_v = localToCamera * vec4(ve_Position_l, 1.f);
    gl_Position = u_Projection * ex_Position_v;
    ex_Normal_v = localToCamera * vec4(ve_Normal_l, 0.f);

    ex_DiffuseColor  = in_Albedo.rgb;
    ex_SpecularColor = in_Albedo.rgb;
    ex_Opacity       = in_Albedo.a;
}
)#";


inline const GLchar* gFragmentShader = R"#(
#version 420

in vec4 ex_Position_v;
in vec4 ex_Normal_v;
in vec3 ex_DiffuseColor;
in vec3  ex_SpecularColor;
in float ex_Opacity;

uniform vec3 u_LightPosition_v;
uniform vec3 u_LightColor = vec3(0.8, 0.0, 0.8);
uniform vec3 u_AmbientColor = vec3(0.2, 0.0, 0.2);

out vec4 out_Color;

void main(void)
{
    // Everything in camera space
    vec3 light = normalize(u_LightPosition_v - ex_Position_v.xyz);
    vec3 view = vec3(0., 0., 1.);
    vec3 h = normalize(view + light);
    vec3 normal = normalize(ex_Normal_v.xyz); // cannot normalize in vertex shader, as interpolation changes length.
    
    float specularExponent = 32;

    vec3 diffuse = 
        ex_DiffuseColor * (u_AmbientColor + u_LightColor * max(0.f, dot(normal, light)));
    vec3 specular = 
        u_LightColor * ex_SpecularColor * pow(max(0.f, dot(normal, h)), specularExponent);
    vec3 color = diffuse + specular;

    // Gamma correction
    float gamma = 2.2;
    out_Color = vec4(pow(color, vec3(1./gamma)), ex_Opacity);
}
)#";


Renderer::Renderer() :
    mProgram{graphics::makeLinkedProgram({
            {GL_VERTEX_SHADER,   {gVertexShader, "PhongVertexShader"}},
            {GL_FRAGMENT_SHADER, {gFragmentShader, "PhongFragmentShader"}},
        }),
        "PhongLighting"
    }
{
    // Explicitly format program using operator<<, otherwise it is casted to GLuint of the 
    // underlying OpenGL resource.
    SELOG_LG(gRenderLogger, debug)("Program inspection reports {}", fmt::streamed(mProgram));
}


void Renderer::render(const Mesh & aMesh,
                      const InstanceStream & aInstances,
                      const UniformRepository & aUniforms,
                      const UniformBlocks & aUniformBlocks)
{
    auto depthTest = graphics::scopeFeature(GL_DEPTH_TEST, true);

    setUniforms(aUniforms, mProgram);
    setBlocks(aUniformBlocks, mProgram);

    //graphics::ScopedBind boundVAO{aMesh.mStream.mVertexArray};
    bind(mVertexArrayRepo.get(aMesh, aInstances, mProgram));

    graphics::use(mProgram);
    glDrawArraysInstanced(GL_TRIANGLES, 0, aMesh.mStream.mVertexCount, aInstances.mInstanceCount);
}


} // namespace snac
} // namespace ad