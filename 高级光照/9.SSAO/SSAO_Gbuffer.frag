#version 330 core
layout (location = 0) out vec4 gPositionDepth;
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
uniform int forceWhite = 0; // 1 = 白模，忽略模型自带贴图

const float NEAR = 0.1;
const float FAR = 100.0f;
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR));
}

void main()
{
    gPositionDepth.xyz = FragPos;
    gPositionDepth.a = LinearizeDepth(gl_FragCoord.z);
    gNormal = normalize(Normal);

    vec3 albedo;
    float spec;
    if (forceWhite == 1)
    {
        albedo = vec3(1.0);
        spec = 0.0;
    }
    else
    {
        albedo = texture2D(material.texture_diffuse1, UVs).rgb;
        spec = texture2D(material.texture_specular1, UVs).r;
    }
    gAlbedoSpec.rgb = albedo;
    gAlbedoSpec.a = spec;
}
