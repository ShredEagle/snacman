#pragma once

#include <renderer/Uniforms.h>
#include <renderer/Shading.h>
#include <renderer/VertexSpecification.h>


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


const char * gFragmentShaderTex = R"#(
    #version 420

    in vec2 ex_Uv;

    layout(binding=0) uniform sampler2D u_Texture;

    uniform int u_SourceChannel;

    uniform unsigned int u_Linearization;
    uniform float u_NearDistance;
    uniform float u_FarDistance;

    out vec4 out_Color;

    void main()
    {
        vec4 sampled = texture(u_Texture, ex_Uv);
        vec4 color = sampled;
        if(u_SourceChannel >= 0)
        {
            color = vec4(vec3(sampled[u_SourceChannel]), 1.0);
        }

        if(u_Linearization == 1)
        {
            // see: http://glampert.com/2014/01-26/visualizing-the-depth-buffer/
            float linearDepth = 
                (2 * u_NearDistance) 
                / (u_FarDistance + u_NearDistance - color.r * (u_FarDistance - u_NearDistance));
            
            color = vec4(vec3(linearDepth), 1.0);
        }
        else if(u_Linearization == 2)
        {
            // ATTENTION: This results in a black screen, this method does not seem to work with our projection

            // see: http://web.archive.org/web/20130416194336/http://olivers.posterous.com/linear-depth-in-glsl-for-real

            // see: http://glampert.com/2014/01-26/visualizing-the-depth-buffer/
            float linearDepth = 
                (2 * u_NearDistance * u_FarDistance) 
                / (u_FarDistance + u_NearDistance - (2 * color.r - 1) * (u_FarDistance - u_NearDistance));
            
            color = vec4(vec3(linearDepth), 1.0);
        }

        //
        // Gamma correction
        //
        float gamma = 2.2;
        // Gamma compression from linear color space to "simple sRGB", as expected by the monitor.
        // (The monitor will do the gamma expansion.)
        out_Color = vec4(pow(color.xyz, vec3(1./gamma)), color.w);
    }
)#";


struct DrawQuadParameters
{
    enum Linearization : GLuint
    {
        None = 0,
        DepthMethod1 = 1,
        DepthMethod2 = 2,
    };

    GLuint mTextureUnit = 0;
    GLint mSourceChannel = -1; // use all channels
    Linearization mLinearization = None;
    GLfloat mNearDistance;
    GLfloat mFarDistance;
};

inline void drawQuad(DrawQuadParameters aParameters = {})
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

    graphics::setUniform(program, "u_SourceChannel", aParameters.mSourceChannel);
    graphics::setUniform(program, "u_Linearization", aParameters.mLinearization);
    graphics::setUniform(program, "u_NearDistance",  aParameters.mNearDistance);
    graphics::setUniform(program, "u_FarDistance",   aParameters.mFarDistance);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


} // namespace ad::renderer