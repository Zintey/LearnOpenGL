#version 330 core
out vec4 FragColor;

in vec2 UVs;

uniform sampler2D screenTexture;

void main()
{
    vec3 color = texture(screenTexture, UVs).rgb;
    float grayscale = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
    FragColor = vec4(vec3(grayscale), 1.0);
}