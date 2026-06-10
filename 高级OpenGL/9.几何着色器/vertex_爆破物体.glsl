#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aUV;

uniform mat4 model;

out VS_OUT {
    vec2 texCoords;
} vs_out;

void main()
{
    gl_Position = model * vec4(aPos, 1.0);
    vs_out.texCoords = aUV;
}