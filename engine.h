#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "types.h"
#include "inputhandler.h"
#include "effects.h"

class Engine {
public:
    Engine();
    ~Engine();

    bool Initialize();
    void Run();

private:
    bool InitResources();
    void UpdateFPSCounter(SDL_GPUCommandBuffer* cmd);
    void RenderFrame(SDL_GPUCommandBuffer* cmd);
    void Cleanup();
     // Environmental Visual Effects Configuration
    Effects m_effects;

    // Context & Windows
    SDL_Window* m_window = nullptr;
    SDL_GPUDevice* m_gpuDevice = nullptr;
    TTF_Font* m_font = nullptr;

    // Static Mesh & Texture Assets
    SDL_GPUBuffer* m_vertexBuffer = nullptr;
    SDL_GPUBuffer* m_indexBuffer = nullptr;
    SDL_GPUTexture* m_textTexture = nullptr;
    SDL_GPUSampler* m_sampler = nullptr;
    SDL_GPUBuffer* m_quadVB = nullptr;
    SDL_GPUBuffer* m_quadIB = nullptr;
    SDL_GPUTexture* m_depthTexture = nullptr;

    // Graphics Pipelines
    SDL_GPUGraphicsPipeline* m_cubePipeline = nullptr;
    SDL_GPUGraphicsPipeline* m_textPipeline = nullptr;

    // Shaders
    SDL_GPUShader* m_cubeVS = nullptr;
    SDL_GPUShader* m_cubeFS = nullptr;
    SDL_GPUShader* m_textVS = nullptr;
    SDL_GPUShader* m_textFS = nullptr;

    // Persistent Live FPS Data Assets
    Uint64 m_lastFpsUpdateTime = 0;
    Uint32 m_frameCount = 0;
    float m_currentFps = 0.0f;
    bool m_hasFpsData = false;

    const Uint32 m_fpsTexW = 256;
    const Uint32 m_fpsTexH = 64;

    SDL_GPUTexture* m_fpsTexture = nullptr;
    SDL_GPUTransferBuffer* m_fpsTexStaging = nullptr;
    SDL_GPUBuffer* m_fpsVB = nullptr;
    SDL_GPUBuffer* m_fpsIB = nullptr;
    SDL_GPUTransferBuffer* m_fpsGeomStaging = nullptr;

    // Runtime state variables
    bool m_running = false;
    float m_rotation = 0.0f;

        // Input Handling
    InputHandler m_inputHandler;

    // Frame timing tracking
    Uint64 m_lastFrameTime = 0;

   

};
