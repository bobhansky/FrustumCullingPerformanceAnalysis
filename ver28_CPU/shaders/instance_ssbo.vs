#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in uint aIndex;

layout(std430, binding = 0) buffer ModelMatrices {
    mat4 allModeMatrix[];
};

out vec2 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = projection * view * allModeMatrix[aIndex] * vec4(aPos, 1.0f); 
}