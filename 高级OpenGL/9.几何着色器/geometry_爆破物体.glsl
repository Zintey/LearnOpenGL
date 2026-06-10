#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec2 texCoords;
} gs_in[];

uniform mat4 view;
uniform mat4 projection;
uniform float time;

out vec2 texCoords;

vec3 getNormal();
vec4 explode(vec4 position, vec3 direction);

void main()
{
    vec3 normal = getNormal();
    for (int i = 0; i < 3; i++)
    {
        gl_Position = projection * view * explode(gl_in[i].gl_Position, normal);
        texCoords = gs_in[i].texCoords;
        EmitVertex();
    }

    EndPrimitive();
}

vec3 getNormal()
{
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[1].gl_Position) - vec3(gl_in[2].gl_Position);
    return normalize(cross(a, b));
}

vec4 explode(vec4 position, vec3 direction)
{
    float magnitude = 2.0;
    direction = direction * ((sin(time * 2.0) + 1.0) / 2.0) * magnitude; 
    return position + vec4(direction, 0.0);
}