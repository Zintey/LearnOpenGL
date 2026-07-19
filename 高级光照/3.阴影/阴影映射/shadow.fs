#version 330 core
out vec4 FragColor;
in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

struct Light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    sampler2D diffuse_texture;

    float shininess;
};

uniform sampler2D shadowMap;
uniform Light light;
uniform vec3 cameraPos;
uniform Material material;
uniform float baseBias;
uniform bool pcf;

float ShadowCalculation(vec4 FragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projPos = FragPosLightSpace.xyz / FragPosLightSpace.w;
    projPos = projPos * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projPos.xy).r;
    float currentDepth = projPos.z;

    float shadow = 0.0;
    float bias = max(baseBias * 0.1, baseBias * (1.0 - dot(normal, lightDir)));
    if (pcf)
    {
        vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
        for(int x = -1; x <= 1; ++x)
        {
            for(int y = -1; y <= 1; ++y)
            {
                float pcfDepth = texture(shadowMap, projPos.xy + vec2(x, y) * texelSize).r; 
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
            }    
        }
        shadow /= 9.0;
    }
    else
    {
        shadow = (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
    }
    if (projPos.z > 1.0)
        shadow = 0.0;
    return shadow;
}

void main()
{
    vec3 color = vec3(texture(material.diffuse_texture, fs_in.TexCoords));

    vec3 ambient = light.ambient * color;
    
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightDir = normalize(-light.direction);

    float diff = max(0.0, dot(lightDir, normal));
    vec3 diffuse = diff * color * light.diffuse;

    vec3 viewDir = normalize(cameraPos - fs_in.FragPos);
    float spec = 0.0;
    vec3 halfwayDir = normalize(viewDir + lightDir);
    spec = pow(max(0.0, dot(halfwayDir, normal)), material.shininess);
        
    vec3 specular = spec * light.specular;
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, normal, lightDir);

    vec3 col = ambient  + (1.0 - shadow) * (diffuse + specular) ;
    
    col = pow(col, vec3(1.0 / 2.2));
    FragColor = vec4( col,1.0);
}