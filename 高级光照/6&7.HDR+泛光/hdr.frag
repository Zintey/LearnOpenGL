#version 330 core

uniform sampler2D tex;
uniform float exposure = 1.0;
out vec4 FragColor;
in vec2 UVs;

void main()
{
    const float gamma = 2.2;
    vec3 color = texture(tex, UVs).rgb;

    // hdr
    color = vec3(1) - exp(-color * exposure);
    
    // gamma
    color = pow(color, vec3(1.0 / gamma) );
    FragColor = vec4(color, 1.0);
}