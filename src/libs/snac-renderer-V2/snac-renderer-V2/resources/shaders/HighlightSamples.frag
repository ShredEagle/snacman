// Write a highlight color in an image at the position a sample would be taken
// (initially implemented to debug importanceSampleGGX())

#version 430


#include "HelpersIbl.glsl"


layout(binding = 1, rgba32f) uniform writeonly image2D outImage;

uniform vec3 u_SurfaceNormal;
uniform float u_AlphaSquared;

void main()
{
    //highlightSamples(u_SurfaceNormal, u_AlphaSquared, outImage);
    vec3 N = u_SurfaceNormal;
    float aAlphaSquared = u_AlphaSquared;

    ivec2 size = imageSize(outImage);

    const uint NumSamples = 512;

    vec3 R = N;
    vec3 V = N;
    
    // Grey out (to test writes)
    for(int h = 0; h < size.y; ++h)
    {
        for(int w = 0; w < size.x; ++w)
        {
            imageStore(outImage, ivec2(w, h), vec4(0.2, 0.2, 0.2, 1.0));
        }
    }

    for(uint i = 0; i < NumSamples; i++)
    {
        vec2 Xi = hammersley(i, NumSamples);
        vec3 H = importanceSampleGGX(Xi, aAlphaSquared, N);
        vec3 L = reflect(-V, H);
        float nDotL = dotPlus(N, L);
        if(nDotL > 0)
        {
            float sc, tc, ma;
            // Cubemap sampling expects left-handed (+Z into screen), so convert
            vec3 dir = vec3(L.xy, -L.z);
            if(abs(dir.z) >= abs(dir.x) && abs(dir.z) >= abs(dir.y))
            {
                if(dir.z > 0)
                {
                    sc = dir.x;
                    tc = -dir.y;
                }

                ma = abs(dir.z);
            }

            ivec2 sampleLocation_pixel = 
                ivec2(
                    vec2((sc / ma + 1.) / 2.,
                         (tc / ma + 1.) / 2.)
                    * vec2(size));

            // Dump 4 pixels, to make the locations bigger.
            imageStore(outImage, sampleLocation_pixel, vec4(1.0, 0.0, 1.0, 1.0));
            imageStore(outImage, sampleLocation_pixel + ivec2(0, 1), vec4(1.0, 0.0, 1.0, 1.0));
            imageStore(outImage, sampleLocation_pixel + ivec2(1, 1), vec4(1.0, 0.0, 1.0, 1.0));
            imageStore(outImage, sampleLocation_pixel + ivec2(1, 0), vec4(1.0, 0.0, 1.0, 1.0));
        }
    }
}
