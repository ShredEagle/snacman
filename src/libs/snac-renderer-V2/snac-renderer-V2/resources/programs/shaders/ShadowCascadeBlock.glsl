layout(std140, binding = 6) uniform ShadowCascadeBlock
{
    bool debugTintCascade;
    // #cascade_hardcode_4
    vec4 cascadeFarPlaneDepths_view;
};


// Interval-based cascade selection
uint selectShadowCascade_IntervalBased(vec3 aFragPosition_cam)
{
    // see: https://learn.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps#interval-based-cascade-selection
    vec4 fragmentDepth_view = vec4(ex_Position_cam.z);
    // A vector with 1 where the pixel depth exceeds the cascade far plane
    // Note: we could probably do away with vec3 here
    // #cascade_hardcode_4
    vec4 depthComparison = vec4(lessThan(fragmentDepth_view, cascadeFarPlaneDepths_view));
    const vec4 availableCascadeIndices = vec4(1, 1, 1, 0);
    return uint(dot(availableCascadeIndices, depthComparison));
}
