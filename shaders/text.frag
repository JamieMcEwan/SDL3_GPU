#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;
// Fragment sampling uses Set 2 in SDL3 GPU
layout(set = 2, binding = 0) uniform sampler2D textSampler;
void main() {

    // 💡 THE CRITICAL VERTICAL FLIP:
    // Invert the Y texture coordinate before sampling the text image array
    vec2 flippedUV = vec2(fragUV.x, 1.0 - fragUV.y);

    outColor = texture(textSampler, fragUV);
}
