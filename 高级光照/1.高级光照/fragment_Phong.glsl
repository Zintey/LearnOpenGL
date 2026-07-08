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

uniform PointLight light;
uniform vec3 cameraPos;
uniform Material material;


void main()
{
    vec3 color = vec3(texture(material.diffuse_texture, UVs));
    float dis = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * dis + light.quadratic * dis * dis);

    vec3 ambient = light.ambient * color;
    
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);

    float diff = max(0.0, dot(lightDir, normal));
    vec3 diffuse = diff * color * light.diffuse;

    vec3 viewDir = normalize(cameraPos - FragPos);
    float spec = 0.0;
    vec3 reflectDir = reflect(-lightDir, normal);
    spec = pow(max(0.0, dot(reflectDir, viewDir)), material.shininess);
    vec3 specular = spec * light.specular;

    FragColor = vec4((ambient + diffuse + specular) * attenuation, 1.0);
}