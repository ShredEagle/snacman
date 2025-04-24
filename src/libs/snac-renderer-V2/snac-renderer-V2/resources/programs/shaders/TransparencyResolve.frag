#version 420

#include "Helpers.glsl"


// see: https://casual-effects.blogspot.com/2015/03/implemented-weighted-blended-order.html#2d_compositing_pass

// sum(rgb * a, a)
uniform sampler2D u_AccumTexture;

// prod(1 - a)
uniform sampler2D u_RevealageTexture;

layout(location = 0) out vec4 out_Color;

void main() 
{
    ivec2 coords = ivec2(gl_FragCoord.xy);

    float revealage = texelFetch(u_RevealageTexture, coords, 0).r;
    if (revealage == 1.0) {
        // Save the blending and color texture fetch cost
        discard; 
    }

    vec4 accum = texelFetch(u_AccumTexture, coords, 0);
    // Suppress overflow
    if (isinf(maxCw(abs(accum)))) {
        accum.rgb = vec3(accum.a);
    }
    vec3 averageColor = accum.rgb / max(accum.a, 0.00001);

    // dst' =  (accum.rgb / accum.a) * (1 - revealage) + dst * revealage
    // TODO could we replace the `1 - revealage` with `revealge` by inverting the src and dst blend?
    out_Color = vec4(averageColor, 1.0 - revealage);
}
