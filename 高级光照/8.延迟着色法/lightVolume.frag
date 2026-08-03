#version 330 core

out vec4 FragColor;

uniform int show_mode;
// 0 -> lighting
// 1 -> position
// 2 -> normal
// 3 -> abedo
// 4 -> spec


struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

uniform PointLight light;
uniform vec3 cameraPos;
uniform vec2 screen_size;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;


vec3 calcLight(PointLight light, vec3 FragPos, vec3 Normal, vec3 Albedo, float Spec)
{
    vec3 color = Albedo;
    float dis = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * dis + light.quadratic * dis * dis);

    vec3 ambient = light.ambient * color;

    vec3 normal = normalize(Normal);

    vec3 lightDir = normalize(light.position - FragPos);

    float diff = max(0.0, dot(lightDir, normal));
    vec3 diffuse = diff * color * light.diffuse;

    vec3 viewDir = normalize(cameraPos - FragPos);
    float spec = 0.0;
    vec3 halfwayDir = normalize(viewDir + lightDir);
    spec = pow(max(0.0, dot(halfwayDir, normal)), 64);
    vec3 specular = spec * light.specular * Spec;

    return (diffuse + specular) * attenuation;
}

void main()
{
    vec3 color = vec3(0.0);
    vec2 uv = gl_FragCoord.xy / screen_size;
    vec3 FragPos = texture(gPosition, uv).rgb;
    vec3 Normal = texture(gNormal, uv).rgb;
    vec3 Abedo = texture(gAlbedoSpec, uv).rgb;
    float Spec = texture(gAlbedoSpec, uv).a;
    switch (show_mode)
    {
        case 0: // lighting
                color += calcLight(light, FragPos, Normal, Abedo, Spec);
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
    }
    
    FragColor = vec4(color, 1.0);
}
