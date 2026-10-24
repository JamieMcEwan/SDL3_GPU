#pragma once
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
};

struct TextureVertex {
    glm::vec2 pos;
    glm::vec2 uv;
};

// Unified Uniform Buffer Object - Aligned to clean 16-byte blocks
struct FrameUniforms {
    glm::mat4 mvp;            // 64 bytes
    glm::mat4 modelView;      // 64 bytes
    glm::vec4 fogColor;       // 16 bytes
    float fogStart;           // 4 bytes
    float fogEnd;             // 4 bytes
    glm::vec2 packingPadding; // 8 bytes -> Total size = 160 bytes (Clean multiple of 16!)
};

extern const std::vector<Vertex> cubeVertices;
extern const std::vector<uint16_t> cubeIndices;
