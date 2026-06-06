#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
uniform sampler2D screenTexture;
void main()
{
    vec4 color = texture(screenTexture, TexCoords);
    
    float offset = 1.0 / 300.0;
    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), // 左上
        vec2( 0.0f,    offset), // 正上
        vec2( offset,  offset), // 右上
        vec2(-offset,  0.0f),   // 左
        vec2( 0.0f,    0.0f),   // 中
        vec2( offset,  0.0f),   // 右
        vec2(-offset, -offset), // 左下
        vec2( 0.0f,   -offset), // 正下
        vec2( offset, -offset)  // 右下
    );

    // 反色
    // FragColor = vec4(vec3(1.0 - texture(screenTexture, TexCoords)), 1.0);
    
    // 灰度
    // float gray = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    // FragColor = vec4(gray, gray, gray, 1.0);
    
    // 锐化
    // float kernel[9] = float[](
    //     -1, -1, -1,
    //     -1,  9, -1,
    //     -1, -1, -1
    // );
    // vec3 sampleTex[9];
    // for (int i = 0; i < 9; i++)
    //     sampleTex[i] = vec3(texture(screenTexture, TexCoords + offsets[i]));
    // vec3 col = vec3(0.f);
    // for (int i = 0; i < 9; i++)
    //     col += sampleTex[i] * kernel[i];
    // FragColor = vec4(col, 1.0f);

    // 模糊
    // float kernel[9] = float[](
    //     1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0,
    //     2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0,
    //     1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0
    // );
    // vec3 sampleTex[9];
    // for (int i = 0; i < 9; i++)
    //     sampleTex[i] = vec3(texture(screenTexture, TexCoords + offsets[i]));
    // vec3 col = vec3(0.f);
    // for (int i = 0; i < 9; i++)
    //     col += sampleTex[i] * kernel[i];
    // FragColor = vec4(col, 1.0f);

    // 边缘检测
    float kernel[9] = float[](
        1, 1, 1,
        1, -8, 1,
        1, 1, 1
    );
    vec3 sampleTex[9];
    for (int i = 0; i < 9; i++)
        sampleTex[i] = vec3(texture(screenTexture, TexCoords + offsets[i]));
    vec3 col = vec3(0.f);
    for (int i = 0; i < 9; i++)
        col += sampleTex[i] * kernel[i];
    FragColor = vec4(col, 1.0f);

}