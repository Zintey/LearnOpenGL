#version 330 core

out vec4 FragColor;
in vec3 Normal;
in vec2 UVs;
in vec3 FragPos;
in vec3 Tangent;
in vec3 Bitangent;

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

uniform sampler2D normal_texture;
uniform bool useNormalMap;
uniform bool useStepParallaxMap;

uniform sampler2D depth_texture;
uniform float height_scale;
uniform bool useParallaxMap;

vec2 StepParallaxMapping(vec2 texCoords, vec3 viewDir, float heightScale)
{
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, max(0.0, viewDir.z));

    float layerDepth = 1.0 / numLayers;
    vec2 P = viewDir.xy / viewDir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    vec2  currentTexCoords = texCoords;
    float currentDepthMapValue = texture(depth_texture, currentTexCoords).r;
    float currentLayerDepth = 0.0;

    while (currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(depth_texture, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(depth_texture, prevTexCoords).r - (currentLayerDepth - layerDepth);
    float weight = afterDepth / (afterDepth - beforeDepth);
    return mix(currentTexCoords, prevTexCoords, weight);
}

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir, float heightScale)
{
    float height = texture(depth_texture, texCoords).r;
    return texCoords - viewDir.xy / viewDir.z * (height * heightScale);
}

void main()
{
    vec3 normal = normalize(Normal);

    vec2 texUVs = UVs;

    if (useParallaxMap)
    {
        // Build TBN matrix and compute tangent-space view direction
        vec3 T = normalize(Tangent);
        vec3 N = normalize(Normal);
        vec3 B = normalize(Bitangent);
        mat3 TBN = mat3(T, B, N);
        vec3 worldViewDir = cameraPos - FragPos;
        vec3 tangentViewDir = normalize(transpose(TBN) * worldViewDir);
        if (useStepParallaxMap)
            texUVs = StepParallaxMapping(UVs, tangentViewDir, height_scale);
        else
            texUVs = ParallaxMapping(UVs, tangentViewDir, height_scale);
    }

    vec3 color = vec3(texture(material.diffuse_texture, texUVs));
    float dis = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * dis + light.quadratic * dis * dis);

    vec3 ambient = light.ambient * color;

    if (useNormalMap)
    {
        vec3 normalMap = texture(normal_texture, texUVs).rgb;
        normalMap = normalMap * 2.0 - 1.0;

        vec3 T = normalize(Tangent);
        vec3 N = normalize(Normal);
        T = normalize(T - dot(T, N) * N);
        vec3 B = cross(N, T);
        mat3 TBN = mat3(T, B, N);
        normal = normalize(TBN * normalMap);
    }

    vec3 lightDir = normalize(light.position - FragPos);

    float diff = max(0.0, dot(lightDir, normal));
    vec3 diffuse = diff * color * light.diffuse;

    vec3 viewDir = normalize(cameraPos - FragPos);
    float spec = 0.0;
    vec3 halfwayDir = normalize(viewDir + lightDir);
    spec = pow(max(0.0, dot(halfwayDir, normal)), material.shininess);
    vec3 specular = spec * light.specular;

    FragColor = vec4(ambient + (diffuse + specular) * attenuation, 1.0);
}
