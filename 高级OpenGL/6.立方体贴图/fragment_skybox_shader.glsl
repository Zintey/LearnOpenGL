#version 330 core

in vec3 TexCoords;

out vec4 FragColor;

uniform samplerCube skybox_texture;

void main()
{
    FragColor = texture(skybox_texture, TexCoords);
}