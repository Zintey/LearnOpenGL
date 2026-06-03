#version 330 core

struct Material {
    // vec3 ambient;
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};
struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
in vec2 TexCoords;
in vec3 FragPos;
in vec3 normal;
out vec4 FragColor;

uniform vec3 cameraPos;
uniform Material material;
uniform Light light;

void main()
{
    // 环境光
    vec3 ambient =  vec3(texture(material.diffuse, TexCoords)) * light.ambient;
    // 漫反射
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(texture(material.diffuse, TexCoords)) * light.diffuse;
    // 镜面反射
    vec3 h = normalize(lightDir + normalize(cameraPos - FragPos));
    float spec = pow(max(dot(norm, h), 0.0), material.shininess);
    vec3 specular =  spec * vec3(texture(material.specular, TexCoords)) * light.specular;
    FragColor = vec4(ambient + diffuse + specular, 1.0);
}