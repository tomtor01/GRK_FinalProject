#version 430 core

layout(location = 0) in vec3 aPos;
out vec2 TexCoord;

uniform mat4 transformation;

void main()
{
    gl_Position = transformation * vec4(aPos, 1.0);
    
    // Normalize vertex position to use for spherical UV mapping
    vec3 normPos = normalize(aPos);

    // Compute U coordinate and ensure it wraps correctly
    float u = atan(normPos.z, normPos.x) / (2.0 * 3.14159265);
    if (u < 0.0) u += 1.0;  // Ensure U is always in range [0,1]

    // Compute V coordinate normally
    float v = 0.5 - asin(normPos.y) / 3.14159265;

    TexCoord = vec2(u, v);
}
