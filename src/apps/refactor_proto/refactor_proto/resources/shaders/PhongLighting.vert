#version 420

in vec3 ve_Position_local;
in vec3 ve_Normal_local;
in vec2 ve_Uv;

in mat4 in_LocalToWorld;
// Will be required to support non-uniform scaling.
//in mat4 in_LocalToWorldInverseTranspose;
in uint in_MaterialIdx;

// WARNING: for some reason, the GLSL compiler assigns the same implicit binding
// index to both uniform blocks if we do not set it explicitly.
layout(std140, binding = 0) uniform ViewBlock
{
    mat4 worldToCamera;
    mat4 projection;
    mat4 viewingProjection;
};

out vec3 ex_Position_cam;
out vec3 ex_Normal_cam;
out vec2[4] ex_Uv; // simulates 4 UV channels, not implement atm
out flat uint ex_MaterialIdx;

void main(void)
{
    mat4 localToWorld = 
        in_LocalToWorld
//#ifdef RIGGING
//        * assembleSkinningMatrix()
//#endif
    ;

    mat4 localToCamera = worldToCamera * localToWorld;
    vec4 position_cam = localToCamera * vec4(ve_Position_local, 1.f);
    gl_Position = projection * position_cam;
    ex_Position_cam = vec3(position_cam);
    ex_Normal_cam = mat3(localToCamera) * ve_Normal_local;
    ex_Uv[0] = ve_Uv; // only 1 uv channel input at the moment
    ex_MaterialIdx = in_MaterialIdx;
}
