#include "DrawQuad.h"

#include <renderer/ScopeGuards.h>
#include <renderer/Shading.h>
#include <renderer/Uniforms.h>
#include <renderer/VertexSpecification.h>


namespace ad::renderer {


namespace {

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

        layout(binding=0) uniform sampler2D u_Texture2D;
        layout(binding=1) uniform sampler2DArray u_Texture2DArray;
        layout(binding=2) uniform samplerCube u_TextureCubemap;

        uniform int u_SourceChannel;

        uniform unsigned int u_Linearization;
        uniform float u_NearDistance;
        uniform float u_FarDistance;
        uniform unsigned int u_Sampler;

        out vec4 out_Color;

        void main()
        {
            vec4 sampled;
            switch(u_Sampler)
            {
                case 0:
                {
                    sampled = texture(u_Texture2D, ex_Uv);
                    break;
                }
                case 1:
                {
                    // TODO allow to select the array index
                    sampled = texture(u_Texture2DArray, vec3(ex_Uv, 0.));
                    break;
                }
                case 2: 
                {
                    // Using Z = +1 will index into the cubemap X_POSITIVE face
                    // (since (u, v) is in [0, 1]^2)
                    // TODO allow to select other cubemap faces.
                    sampled = textureLod(u_TextureCubemap, vec3(ex_Uv * 2 - 1, 1.), 1); // TODO control lod with uniform
                    break;
                }
            }

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
            else if(u_Linearization == 3)
            {
                // see: https://casual-effects.blogspot.com/2015/03/implemented-weighted-blended-order.html#2d_compositing_pass
                color = vec4(color.rgb / max(color.a, 0.00001), 1.0);
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


} // unnamed namespace


void drawQuad(DrawQuadParameters aParameters)
{
    // IMPORTANT: The code is not managing the lifetime of the static GL objects (no RAII wrappers) by design.
    //  Because static objects are destroyed after the main() stack is unwound, the GL destructors 
    //  would be called *after* the GL context is destroyed.
    // i.e., these objects are implicitly destroyed when the GL context is destoyed,
    // which is almost exactly what we need anyway.
    // A drawback is that by using naked GLuint instead of our types, we lose our helper functions.
    static GLuint dummyVao;
    glGenVertexArrays(1, &dummyVao);
    static GLuint program = 
            graphics::makeLinkedProgram({
                {GL_VERTEX_SHADER, gVertexShaderTex},
                {GL_FRAGMENT_SHADER, gFragmentShaderTex},
            }).release();

    // Sadly a non zero VAO must be bound for any draw call
    glBindVertexArray(dummyVao);
    glUseProgram(program);

    glProgramUniform1i( program, glGetUniformLocation(program, "u_SourceChannel"), aParameters.mSourceChannel);
    glProgramUniform1ui(program, glGetUniformLocation(program, "u_Linearization"), aParameters.mOperation);
    glProgramUniform1f( program, glGetUniformLocation(program, "u_NearDistance"),  aParameters.mNearDistance);
    glProgramUniform1f( program, glGetUniformLocation(program, "u_FarDistance"),   aParameters.mFarDistance);
    glProgramUniform1ui(program, glGetUniformLocation(program, "u_Sampler"),       aParameters.mSampler);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Reset default program, to avoid potential issues when calling glClear() with warnings such as:
    // > "The current GL state uses a sampler (0) that has depth comparisons enabled, with a texture object (shadow_map) with a depth format,"
    // > " by a shader that samples it with a non-shadow sampler. Using this state to sample would result in undefined behavior"
    // Yes, glClear() should not validate anything related to the active program, but who am I to judge the driver.
    glUseProgram(0);
}


QuadDrawer::QuadDrawer(graphics::ShaderSourceView aFragmentCode) :
    mProgram{graphics::makeLinkedProgram({
        {GL_VERTEX_SHADER, gVertexShaderTex},
        {GL_FRAGMENT_SHADER, aFragmentCode},
    })}
{}


void QuadDrawer::drawQuad() const
{
    // A non zero VAO must be bound for any draw call
    graphics::ScopedBind boundVao{mDummyVao};
    graphics::ScopedBind boundProgram{mProgram};

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


} // namespace ad::renderer