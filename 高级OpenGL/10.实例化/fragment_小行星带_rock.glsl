#version 330 core
in vec3 Normal;
in vec2 TexCoords;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
};

uniform Material material;
out vec4 FragColor;

void main()
{
    FragColor = texture(material.texture_diffuse1, TexCoords);
}