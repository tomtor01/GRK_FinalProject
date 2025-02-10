#version 430 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D colorTexture;

void main(){
    FragColor = texture(colorTexture, TexCoord);
}
