// Write a highlight color in an image at the position a sample would be taken
// (initially implemented to debug importanceSampleGGX())

#version 430


#include "HelpersIbl.glsl"


layout(binding = 1, rgba32f) uniform writeonly image2D outImage;

uniform vec3 u_SurfaceNormal;
uniform float u_Roughness;

vec3 getLight_importanceSampleGGX(uint i, uint aNumSamples, vec3 N, float aAlphaSquared)    
{
    vec3 R = N;
    vec3 V = N;
    
    vec2 Xi = hammersley(i, aNumSamples);
    vec3 H = importanceSampleGGX(Xi, aAlphaSquared, N);
    vec3 L = reflect(-V, H);

    return L;
}


vec3 getLight_importanceSampleCosDir(uint i, uint aNumSamples, vec3 N)    
{
    vec2 Xi = hammersley(i, aNumSamples);
    return importanceSampleCosDir(Xi, N);
}


void main()
{
    float alphaSquared = pow(alphaFromRoughness(u_Roughness), 2);

    // Note: It seems "tricky" to have a function taking an image as parameter
    // see: https://github.com/KhronosGroup/glslang/issues/1720
    //highlightSamples(u_SurfaceNormal, alphaSquared, outImage);

    vec3 N = u_SurfaceNormal;

    ivec2 size = imageSize(outImage);

    // Grey out (to test writes)
    for(int h = 0; h < size.y; ++h)
    {
        for(int w = 0; w < size.x; ++w)
        {
            imageStore(outImage, ivec2(w, h), vec4(0.2, 0.2, 0.2, 1.0));
        }
    }

    const uint NumSamples = 512;

    for(uint i = 0; i < NumSamples; i++)
    {
#if 0 // Specular importance sampling
        vec3 L = getLight_importanceSampleGGX(i, NumSamples, N, alphaSquared);
#else // Diffuse importance sampling
        vec3 L = getLight_importanceSampleCosDir(i, NumSamples, N);
#endif
        float nDotL = dotPlus(N, L);
        if(nDotL > 0)
        {
            float sc, tc, ma;
            // Cubemap sampling expects left-handed (+Z into screen), so convert
            vec3 dir = worldToCubemap(L);
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
