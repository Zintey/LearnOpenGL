#version 330 core

out vec4 FragColor;
in vec2 UVs;

uniform int show_mode;
// 0 -> lighting
// 1 -> position
// 2 -> normal
// 3 -> abedo
// 4 -> spec
// 5 -> ao (ssao贴图)


struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

const int LIGHT_CNT = 4;
uniform PointLight light[LIGHT_CNT];
uniform int lightEnabled[LIGHT_CNT] = int[LIGHT_CNT](1, 1, 1, 1); // 每个光源是否开启
uniform vec3 cameraPos;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D ssao;
uniform int useSSAO = 1; // 是否启用 SSAO（0 时 AO 恒为 1.0）

vec3 calcLight(PointLight light, vec3 FragPos, vec3 Normal, vec3 Albedo, float Spec)
{
    vec3 color = Albedo;
    float dis = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * dis + light.quadratic * dis * dis);

    float ao = mix(1.0, texture(ssao, UVs).r, useSSAO);
    vec3 ambient = light.ambient * color * ao;

    vec3 normal = normalize(Normal);

    vec3 lightDir = normalize(light.position - FragPos);

    float diff = max(0.0, dot(lightDir, normal));
    vec3 diffuse = diff * color * light.diffuse;

    vec3 viewDir = normalize(cameraPos - FragPos);
    float spec = 0.0;
    vec3 halfwayDir = normalize(viewDir + lightDir);
    spec = pow(max(0.0, dot(halfwayDir, normal)), 64);
    vec3 specular = spec * light.specular * Spec;

    return ambient + (diffuse + specular) * attenuation;
}

void main()
{
    vec3 color = vec3(0.0);
    vec3 FragPos = texture(gPosition, UVs).rgb;
    vec3 Normal = texture(gNormal, UVs).rgb;
    vec3 Abedo = texture(gAlbedoSpec, UVs).rgb;
    float Spec = texture(gAlbedoSpec, UVs).a;
    switch (show_mode)
    {
        case 0: // lighting
            for (int i = 0; i < LIGHT_CNT; i++)
            {
                if (lightEnabled[i] == 0) continue;
                color += calcLight(light[i], FragPos, Normal, Abedo, Spec);
            }
        break;
        case 1: // position
            color = FragPos;
        break;
        case 2: // normal
        
            color = Normal;
        break;
        case 3: // abedo
            color = Abedo;
        break;
        case 4: // spec
            color = vec3(Spec);
        break;
        case 5: // ao (ssao贴图)
            color = vec3(texture(ssao, UVs).r);
        break;
    }
    
    FragColor = vec4(color, 1.0);
}
