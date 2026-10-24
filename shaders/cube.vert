#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPos;

layout(std140, set = 1, binding = 0) uniform FrameBuffer {
    mat4 mvp;
    mat4 modelView;
    vec4 fogColor;
    float fogStart;
    float fogEnd;
    vec2 padding;
} registers;

void main() {
    gl_Position = registers.mvp * vec4(inPos, 1.0);
    fragColor = inColor;
    
    // Transform vertex position into true View Space (Camera Relative)
    fragPos = (registers.modelView * vec4(inPos, 1.0)).xyz;
}