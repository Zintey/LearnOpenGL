#version 330 core

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;
in vec3 Normal;
in vec2 UVs;
in vec3 FragPos;

struct PointLight {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct Material {
    sampler2D diffuse_texture;

    float shininess;
};

uniform PointLight light;
uniform vec3 cameraPos;
uniform Material material;
uniform float bloomThreshold = 1.0;

void main()
{
    vec3 normal = normalize(Normal);
    vec3 color = vec3(texture(material.diffuse_texture, UVs));
    vec3 viewDir = normalize(cameraPos - FragPos);

    vec3 lightDir = normalize(light.position - FragPos);
    float dis = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * dis + light.quadratic * dis * dis);

    vec3 ambient = light.ambient * color;

    float diff = max(0.0, dot(lightDir, normal));
    vec3 diffuse = diff * color * light.diffuse;

    vec3 halfwayDir = normalize(viewDir + lightDir);
    float spec = pow(max(0.0, dot(halfwayDir, normal)), material.shininess);
    vec3 specular = spec * light.specular;

    vec3 result = ambient + (diffuse + specular) * attenuation;
    FragColor = vec4(result, 1.0);

    // Check if fragment brightness exceeds threshold for bloom
    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > bloomThreshold)
        BrightColor = vec4(result, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
