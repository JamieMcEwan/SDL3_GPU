#include "shaderloader.h"
#include <iostream>

SDL_GPUShader* LoadShader(
    SDL_GPUDevice* device, 
    const char* filepath, 
    SDL_GPUShaderStage stage, 
    Uint32 numSamplers, 
    Uint32 numUniformBuffers) {
    size_t size;
    void* code = SDL_LoadFile(filepath, &size);
    if (!code) {
        std::cerr << "Failed to read shader file: " << filepath << std::endl;
        return nullptr;
    }
    SDL_GPUShaderCreateInfo createInfo = {};
    createInfo.code = (const Uint8*)code;
    createInfo.code_size = size;
    createInfo.entrypoint = "main";
    createInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    createInfo.stage = stage;
    createInfo.num_samplers = numSamplers;
    createInfo.num_uniform_buffers = numUniformBuffers;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &createInfo);
    SDL_free(code);
    return shader;
}
