#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;

uniform mat4 model;

out VS_OUT {
    vec3 normal;
} vs_out;

void main()
{
    gl_Position = model * vec4(aPos, 1.0);
    vs_out.normal = normalize(mat3(transpose(inverse(model))) * aNorm);
}