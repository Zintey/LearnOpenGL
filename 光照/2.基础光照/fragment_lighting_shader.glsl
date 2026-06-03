#version 330 core
in vec3 FragPos;
in vec3 normal;

out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 cameraPos;

void main()
{
    // 环境光
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    // 漫反射
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    // 镜面反射
    float specularStrength = 0.5;
    vec3 h = normalize(lightDir + normalize(cameraPos - FragPos));
    float spec = pow(max(dot(norm, h), 0.0), 128);
    vec3 specular = specularStrength * spec * lightColor;
    FragColor = vec4((ambient + diffuse + specular) * objectColor, 1.0);
}