#version 330 core
layout (location = 0) out vec3 gPositon;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 UVs;
in vec3 FragPos;
in vec3 Normal;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
};

uniform Material material;

void main()
{
    gPositon = FragPos;
    gNormal = normalize(Normal);

    gAlbedoSpec.rgb = texture2D(material.texture_diffuse1, UVs).rgb;
    gAlbedoSpec.a = texture2D(material.texture_specular1, UVs).r;
}
