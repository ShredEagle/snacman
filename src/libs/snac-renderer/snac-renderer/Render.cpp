#include "Render.h"

namespace ad {
namespace snac {


inline const GLchar* gVertexShader = R"#(
#version 420

layout(location=0) in vec3 ve_VertexPosition_l;

layout(std140) uniform ViewingBlock
{
    mat4 u_Camera;
    mat4 u_Projection;
};

out vec4 ex_Color;

void main(void)
{
    gl_Position = u_Projection * u_Camera * vec4(ve_VertexPosition_l, 1.f);
    ex_Color = vec4(1.f, 0.3, 0.8f, 1.f);
}
)#";


inline const GLchar* gFragmentShader = R"#(
#version 420

in vec4 ex_Color;
out vec4 out_Color;

void main(void)
{
    out_Color = ex_Color;
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
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    constexpr graphics::BindingIndex viewingLocation{1};
    bind(aCamera.mViewing, viewingLocation);

    GLuint viewingBlockIndex;
    if(viewingBlockIndex = glGetUniformBlockIndex(mProgram, "ViewingBlock") == GL_INVALID_INDEX)
    {
        throw std::logic_error{"Invalid block name."};
    }
    glUniformBlockBinding(mProgram, viewingBlockIndex, viewingLocation);

    //graphics::ScopedBind boundVAO{aMesh.mStream.mVertexArray};
    bind(aMesh.mStream.mVertexArray);
    {
        //graphics::ScopedBind boundVBO{aMesh.mStream.mVertexBuffers.front()};
        bind(aMesh.mStream.mVertexBuffers.front());
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
        glEnableVertexAttribArray(0);
    }

    use(mProgram);
    glDrawArrays(GL_TRIANGLES, 0, aMesh.mStream.mVertexCount);
}


} // namespace snac
} // namespace ad
