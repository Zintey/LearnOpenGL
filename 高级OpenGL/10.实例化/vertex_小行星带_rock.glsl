#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aUVs;
layout (location = 3) in mat4 instanceMatrix;

uniform mat4 view;
uniform mat4 projection;

out vec3 Normal;
out vec2 TexCoords;

void main()
{
    gl_Position = projection * view * instanceMatrix * vec4(aPos, 1.0);
    Normal = aNorm;
    TexCoords = aUVs;
}