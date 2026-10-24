#pragma once
#include <SDL3/SDL.h>

SDL_GPUShader* LoadShader(
    SDL_GPUDevice* device, 
    const char* filepath, 
    SDL_GPUShaderStage stage, 
    Uint32 numSamplers, 
    Uint32 numUniformBuffers );
