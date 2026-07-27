#version 330 core

out vec4 FragColor;
in vec3 Normal;
in vec2 UVs;
in vec3 FragPos;
in vec3 Tangent;

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

// ---- Normal mapping uniforms ----
uniform sampler2D normal_texture;
uniform bool useNormalMap;
// --------------------------------

void main()
{
    vec3 color = vec3(texture(material.diffuse_texture, UVs));
    float dis = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * dis + light.quadratic * dis * dis);

    vec3 ambient = light.ambient * color;

    vec3 normal = normalize(Normal);

    // ============================================================
    // TODO: NORMAL MAPPING - 用从法线贴图采样到的逐片段法线替换插值法线
    //
    // Step 1. 采法线贴图:
    //    vec3 normalMap = texture(normal_texture, UVs).rgb;
    //
    // Step 2. 将颜色值从 [0,1] 变换到 [-1,1] 范围:
    //    normalMap = normalMap * 2.0 - 1.0;
    //
    // Step 3. 构建 TBN 矩阵 (Gram-Schmidt 正交化):
    //    vec3 T = normalize(Tangent);
    //    vec3 N = normalize(Normal);
    //    T = normalize(T - dot(T, N) * N);
    //    vec3 B = cross(N, T);
    //    mat3 TBN = mat3(T, B, N);
    //
    // Step 4. 将法线从切线空间变换到世界空间:
    //    normal = normalize(TBN * normalMap);
    //
    // (可选) 用 if (useNormalMap) { ... } 包裹以上代码
    // ============================================================
    if (useNormalMap)
    {
        vec3 normalMap = texture(normal_texture, UVs).rgb;
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
