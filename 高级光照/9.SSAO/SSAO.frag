#version 330 core

layout (location = 0) out float fragColor;
in vec2 texCoords;

uniform sampler2D gPositionDepth;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform int kernelCnt = 64;
uniform float sampleRadius = 1.0;

uniform vec2 texNoiseScale = vec2(480.0, 270.0);

uniform mat4 projection;
uniform mat4 view;

void main()
{
    // gPositionDepth.xyz / gNormal 里存的是世界空间数据
    vec3 fragPositon = texture(gPositionDepth, texCoords).xyz;
    vec3 normal = texture(gNormal, texCoords).rgb;
    vec3 randomVec = texture(texNoise, texCoords * texNoiseScale).xyz;

    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // 当前片元的视图空间 z（负值，朝向 -z）
    vec3 fragView = (view * vec4(fragPositon, 1.0)).xyz;

    float occlusion = 0.0;
    for (int i = 0; i < kernelCnt; i++)
    {
        // 采样点（世界空间）
        vec3 sample = TBN * samples[i];
        sample = fragPositon.xyz + sample * sampleRadius;

        // 世界空间 -> 视图空间 -> 裁剪空间，得到采样点的屏幕位置
        vec4 offset = view * vec4(sample, 1.0);
        float sampleViewZ = offset.z;
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        // 该屏幕位置下表面记录的视图空间 z（负值，由线性深度取负得到）
        float sampleDepth = -texture(gPositionDepth, offset.xy).w;
        float rangeCheck = smoothstep(0.0, 1.0, sampleRadius / abs(fragView.z - sampleDepth));
        occlusion += (sampleDepth >= sampleViewZ ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / kernelCnt);
    fragColor = occlusion;
}
