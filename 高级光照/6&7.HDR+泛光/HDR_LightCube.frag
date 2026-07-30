#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 bloomColor;

uniform vec3 lightColor = vec3(10.0);
uniform float bloomThreshold = 1.0;

void main()
{
    vec3 color = lightColor;
    FragColor = vec4(color, 1.0);

    // Check brightness for bloom output
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > bloomThreshold)
        bloomColor = vec4(color, 1.0);
    else
        bloomColor = vec4(0.0, 0.0, 0.0, 1.0);
}