#include "inputhandler.h"
#include <iostream>

InputHandler::InputHandler() 
    : m_positionOffset(0.0f, 0.0f, 0.0f)
    , m_movementSpeed(2.5f) // Units per second
    , m_keyUpPressed(false)
    , m_keyDownPressed(false) {}

void InputHandler::HandleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.scancode) {
            case SDL_SCANCODE_UP:    m_keyUpPressed = true;   break;
            case SDL_SCANCODE_DOWN:  m_keyDownPressed = true; break;
            default: break;
        }
    } 
    else if (event.type == SDL_EVENT_KEY_UP) {
        switch (event.key.scancode) {
            case SDL_SCANCODE_UP:    m_keyUpPressed = false; break;
            case SDL_SCANCODE_DOWN:  m_keyDownPressed = false; break;
            default: break;
        }
    }
}

void InputHandler::Update(float deltaTime) {
    // Up arrow moves the cube BACK (away from camera -> negative Z)
    if (m_keyUpPressed) {
        m_positionOffset.z -= m_movementSpeed * deltaTime;
    }
    // Down arrow moves the cube FORWARD (towards camera -> positive Z)
    if (m_keyDownPressed) {
        m_positionOffset.z += m_movementSpeed * deltaTime;
    }
}
