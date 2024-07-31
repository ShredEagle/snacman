// WARNING: for some reason, the GLSL compiler assigns the same implicit binding
// index to all uniform blocks if we do not set it explicitly.
layout(std140, binding = 0) uniform ViewProjectionBlock
{
    mat4 worldToCamera;
    mat4 cameraToWorld;
    mat4 projection;
    mat4 viewingProjection;
};
