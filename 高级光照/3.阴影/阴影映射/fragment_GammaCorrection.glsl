#version 330 core

out vec4 FragColor;
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

uniform bool blinn;
uniform bool gamma;

uniform PointLight light;
uniform vec3 cameraPos;
uniform Material material;


void main()
{
    vec3 color = vec3(texture(material.diffuse_texture, UVs));
    float dis = length(light.position - FragPos);
    float attenuation;
    if (gamma)
        attenuation = 1.0 / (dis * dis);
    else
        // attenuation = 1.0 / dis;
        attenuation = 1.0 / (light.constant + light.linear * dis + light.quadratic * dis * dis);

    vec3 ambient = light.ambient * color;
    
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);

    float diff = max(0.0, dot(lightDir, normal));
    vec3 diffuse = diff * color * light.diffuse;

    vec3 viewDir = normalize(cameraPos - FragPos);
    float spec = 0.0;
    if (blinn)
    {
        vec3 halfwayDir = normalize(viewDir + lightDir);
        spec = pow(max(0.0, dot(halfwayDir, normal)), material.shininess);
    }
    else
    {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(0.0, dot(reflectDir, viewDir)), material.shininess);
    }
    vec3 specular = spec * light.specular;

    vec3 col = ambient + (diffuse + specular) * attenuation;
    if (gamma)
        col = pow(col, vec3(1.0 / 2.2));
    FragColor = vec4( col,1.0);
}