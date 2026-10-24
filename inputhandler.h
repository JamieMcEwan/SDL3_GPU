#pragma once
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

class InputHandler {
public:
    InputHandler();
    ~InputHandler() = default;

    // Process incoming SDL events
    void HandleEvent(const SDL_Event& event);
    
    // Updates position based on currently held keys (deltaTime based)
    void Update(float deltaTime);

    // Read-only getter for the position modifier
    const glm::vec3& GetPositionOffset() const { return m_positionOffset; }

private:
    glm::vec3 m_positionOffset;
    float m_movementSpeed;

    // Track active key states
    bool m_keyUpPressed;
    bool m_keyDownPressed;
};
