#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    sampler2D diffuse_texture;

    float shininess;
};

uniform samplerCube shadowMap;
uniform Light light;
uniform vec3 cameraPos;
uniform Material material;
uniform float far_plane;
uniform float baseBias;
uniform bool pcf;
uniform int pcf_level;

// 20 个采样偏移方向，用于立方体贴图 PCF
vec3 sampleOffsetDirections[20] = vec3[]
(
    vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
    vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
    vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
    vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
    vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir)
{
    // 从光源指向片段的向量（同时也是采样立方体贴图的方向）
    vec3 fragToLight = fragPos - light.position;
    float currentDepth = length(fragToLight) / far_plane;

    float shadow = 0.0;
    float bias = max(baseBias * 0.1, baseBias * (1.0 - dot(normal, lightDir)));

    if (pcf)
    {
        // 偏移量随距离增大，避免远距离出现过多锯齿
        float diskRadius = (1.0 + currentDepth * 50.0) / 200.0;
        int samples = 0;
        for (int i = 0; i < 20; ++i)
        {
            float closestDepth = texture(shadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
            shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
            samples++;
        }
        shadow /= float(samples);
    }
    else
    {
        float closestDepth = texture(shadowMap, fragToLight).r;
        shadow = (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
    }

    // 超出远平面的片段不在阴影中
    if (currentDepth > 1.0)
        shadow = 0.0;

    return shadow;
}

void main()
{
    vec3 color = vec3(texture(material.diffuse_texture, fs_in.TexCoords));

    vec3 ambient = light.ambient * color;

    vec3 normal = normalize(fs_in.Normal);
    vec3 lightDir = normalize(light.position - fs_in.FragPos);

    float diff = max(0.0, dot(lightDir, normal));
    vec3 diffuse = diff * color * light.diffuse;

    vec3 viewDir = normalize(cameraPos - fs_in.FragPos);
    float spec = 0.0;
    vec3 halfwayDir = normalize(viewDir + lightDir);
    spec = pow(max(0.0, dot(halfwayDir, normal)), material.shininess);

    vec3 specular = spec * light.specular;
    float shadow = ShadowCalculation(fs_in.FragPos, normal, lightDir);

    vec3 col = ambient + (1.0 - shadow) * (diffuse + specular);

    col = pow(col, vec3(1.0 / 2.2));
    FragColor = vec4(col, 1.0);
}