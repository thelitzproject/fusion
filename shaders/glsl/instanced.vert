#version 450

// Per-vertex (binding 0, rate = vertex)
layout(location = 0) in vec2 inPos;

// Per-instance (binding 1, rate = instance)
layout(location = 1) in vec2 inOffset;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main()
{
    gl_Position = vec4(inPos * 0.15 + inOffset, 0.0, 1.0);
    fragColor   = inColor;
}
