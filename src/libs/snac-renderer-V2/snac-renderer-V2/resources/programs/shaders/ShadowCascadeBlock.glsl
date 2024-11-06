layout(std140, binding = 6) uniform ShadowCascadeBlock
{
    bool debugTintCascade;
    // #cascade_hardcode_4
    vec4 cascadeFarPlaneDepths_view;
};