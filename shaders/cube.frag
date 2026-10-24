#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPos;
layout(location = 0) out vec4 outColor;

layout(std140, set = 3, binding = 0) uniform FrameBuffer {
    mat4 mvp;
    mat4 modelView;
    vec4 fogColor;
    float fogStart;
    float fogEnd;
    vec2 padding;
} registers;

void main() {
    // In Vulkan View Space, positions in front of the camera are negative Z.
    // Negating it gives us a positive linear depth distance value.
    float depth = -fragPos.z;

    // Calculate linear fog depth factor
    float fogFactor = (registers.fogEnd - depth) / (registers.fogEnd - registers.fogStart);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    // Mix vertex colors cleanly into background fog clear color
    vec3 finalColor = mix(registers.fogColor.rgb, fragColor, fogFactor);
    outColor = vec4(finalColor, 1.0);
}