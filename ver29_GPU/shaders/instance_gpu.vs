#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;


// SSBO
layout(std430, binding = 0) buffer ModelMatrices {
    mat4 allModeMatrix[];
};

layout(std430, binding = 2) buffer VisibleIndices {
    uint count;
    uint visibleIndices[];
};

uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoords;

void main()
{
    // get actual instance index from visibility list
    uint realInstanceID = visibleIndices[gl_InstanceID];

    // get model matrix
    mat4 model = allModeMatrix[realInstanceID];

    TexCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}