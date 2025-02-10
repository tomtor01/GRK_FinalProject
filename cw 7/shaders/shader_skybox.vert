#version 430 core
layout(location = 0) in vec3 aPos;
out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main() {
    TexCoords = aPos;
    // Remove translation from the view matrix so the skybox stays centered on the camera:
    mat4 viewNoTrans = mat4(mat3(view));
    gl_Position = projection * viewNoTrans * vec4(aPos, 1.0);
    // Force the depth to 1.0 so the skybox is rendered at the far plane.
    gl_Position.z = gl_Position.w;
}