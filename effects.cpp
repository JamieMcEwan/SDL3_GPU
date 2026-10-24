#include "effects.h"

Effects::Effects() {
    m_fogData.color = glm::vec4(0.1f, 0.1f, 0.15f, 1.0f); // Matches background sky clear
    
    // Widen ranges so it starts clear up close and slowly fades deep into the background
    m_fogData.startDistance = 20.0f; // Fog begins 
    m_fogData.endDistance = 25.0f;   // Cube completely blends out here (well before the 10.0 clip plane!)

    m_fogData.packingPadding = glm::vec2(0.0f, 0.0f);
}
void Effects::SetFogRange(float startDist, float endDist) {
    m_fogData.startDistance = startDist;
    m_fogData.endDistance = endDist;
}

void Effects::SetFogColor(const glm::vec4& color) {
    m_fogData.color = color;
}
