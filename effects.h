#pragma once
#include <glm/glm.hpp>

// This struct must exactly match the packing layout layout inside your fragment shader!
struct FogUniforms {
    glm::vec4 color;          // 16 bytes (Offset 0)
    
    float startDistance;      // 4 bytes  (Offset 16)
    float endDistance;        // 4 bytes  (Offset 20)
    glm::vec2 packingPadding; // 8 bytes  (Offset 24) -> Total structure size = 32 bytes
};


class Effects {
public:
    Effects();
    ~Effects() = default;

    // Read-only getter for the current frame fog parameters
    const FogUniforms& GetFogSettings() const { return m_fogData; }

    // Allows dynamic manipulation of fog density in real-time if needed
    void SetFogRange(float startDist, float endDist);
    void SetFogColor(const glm::vec4& color);

private:
    FogUniforms m_fogData;
};
