#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D texture_reflection1;
};

uniform Material material;
uniform samplerCube skybox;
uniform vec3 cameraPos;

void main()
{
    vec3 diffuseColor = texture(material.texture_diffuse1, TexCoords).rgb;
    
    float reflectionIntensity = texture(material.texture_reflection1, TexCoords).r;
    
    vec3 viewDir = normalize(FragPos - cameraPos);
    vec3 reflectDir = reflect(viewDir, normalize(Normal));
    vec3 reflectColor = texture(skybox, reflectDir).rgb;
    
    FragColor = vec4(diffuseColor + reflectColor * reflectionIntensity, 1.0);
    
}