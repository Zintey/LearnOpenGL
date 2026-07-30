#version 330 core
out vec4 FragColor;
in vec2 UVs;

uniform sampler2D image;
uniform bool horizontal;

const float weights[5] = float[](0.227027, 0.194595, 0.121621, 0.054054, 0.016216);

void main()
{
    vec2 texelSize = 1.0 / textureSize(image, 0);
    vec3 result = texture(image, UVs).rgb * weights[0];
    if (horizontal)
    {
        for (int i = 1; i < 5; i++)
        {
            result += texture(image, UVs + vec2(texelSize.x * i, 0.0)).rgb * weights[i];
            result += texture(image, UVs - vec2(texelSize.x * i, 0.0)).rgb * weights[i];
        }
    }
    else
    {
        for (int i = 1; i < 5; i++)
        {
            result += texture(image, UVs + vec2(0.0, texelSize.y * i)).rgb * weights[i];
            result += texture(image, UVs - vec2(0.0, texelSize.y * i)).rgb * weights[i];
        }
    }
    FragColor = vec4(result, 1.0);
}
