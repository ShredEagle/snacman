#pragma once

#include <renderer/VertexSpecification.h>
#include <renderer/Shading.h>


namespace ad::renderer {


const char * gVertexShaderTex = R"#(
    #version 420

    const vec2 pos_data[4] = vec2[] (
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0,  1.0)
    );

    const vec2 tex_data[4] = vec2[] (
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0)
    );

    out vec2 ex_Uv;

    void main() 
    {
        ex_Uv = tex_data[gl_VertexID];
        gl_Position = vec4(pos_data[gl_VertexID], 0.0, 1.0);
    }
)#";

// TODO normalize options (e.g. linearize depth buffer, min-max clamping, ...)
const char * gFragmentShaderTex = R"#(
    #version 420

    in vec2 ex_Uv;

    layout(binding=0) uniform sampler2D u_Texture;

    out vec4 out_Color;

    void main()
    {
        //out_Color = vec4(0., 1., 0., 1.);

        vec4 color = texture(u_Texture, ex_Uv);
        out_Color = color;
        //vec4 color = vec4(vec3(texture(u_Texture, ex_Uv).r), 1.);
        //out_Color = vec4(1., 1., 1., 1.) - ((vec4(1., 1., 1., 1.) - color) * 4);
    }
)#";


inline void drawQuad()
{
    static graphics::VertexArrayObject dummyVao;
    static graphics::Program program = 
        graphics::makeLinkedProgram({
            {GL_VERTEX_SHADER, gVertexShaderTex},
            {GL_FRAGMENT_SHADER, gFragmentShaderTex},
        });

    // Sadly a non zero VAO must be bound for any draw call
    graphics::ScopedBind boundVao{dummyVao};
    graphics::ScopedBind boundProgram{program};

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


} // namespace ad::renderer