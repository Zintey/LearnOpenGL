#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D depthMap;

uniform float near; 
uniform float far; 

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));    
}

void main()
{             
    float depthValue = texture(depthMap, TexCoords).r;
    
    float linearDepth = LinearizeDepth(depthValue) / far;
    
    FragColor = vec4(vec3(linearDepth), 1.0);
}