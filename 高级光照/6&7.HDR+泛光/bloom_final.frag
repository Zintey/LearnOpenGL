#version 330 core
out vec4 FragColor;
in vec2 UVs;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float exposure;
uniform bool bloom;

void main()
{
    vec3 color = texture(scene, UVs).rgb;
    if (bloom)
    {
        vec3 bloomColor = texture(bloomBlur, UVs).rgb;
        color += bloomColor; // additive blending
    }

    // HDR tone mapping (Reinhard)
    color = vec3(1.0) - exp(-color * exposure);

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
