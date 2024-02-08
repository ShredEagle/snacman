float dotPlus(vec3 a, vec3 b)
{
    return max(0.f, dot(a, b));
}


float maxCw(vec3 v)
{
    return max(max(v.x, v.y), v.z);
}


float maxCw(vec4 v)
{
    //see: https://stackoverflow.com/a/77071476
    vec2 pairs = max(v.xy, v.zw);
    return max(pairs.x, pairs.y);
}