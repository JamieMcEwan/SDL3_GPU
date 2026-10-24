#include "engine.h"
#include "shaderloader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include <algorithm>
#include <iostream>

// Define our constant vertices out-of-line
const std::vector<Vertex> cubeVertices = {
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // Front Face
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, // Back Face
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.2f, 0.2f, 0.2f}}
};

const std::vector<uint16_t> cubeIndices = {
    0, 1, 2, 2, 3, 0,  1, 5, 6, 6, 2, 1,
    7, 6, 5, 5, 4, 7,  4, 0, 3, 3, 7, 4,
    4, 5, 1, 1, 0, 4,  3, 2, 6, 6, 7, 3
};

Engine::Engine() {
    m_lastFpsUpdateTime = SDL_GetTicks();
}

Engine::~Engine() {
    Cleanup();
}

bool Engine::Initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO)) return false;
    if (!TTF_Init()) { SDL_Quit(); return false; }

    m_window = SDL_CreateWindow("SDL3 GPU Partitioned Scene", 800, 600, 0);
    m_gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
    if (!m_gpuDevice || !SDL_ClaimWindowForGPUDevice(m_gpuDevice, m_window)) return false;

    m_lastFrameTime = SDL_GetTicks();


    return InitResources();
}

bool Engine::InitResources() {
    // --- SECTION 1: Build Geometry Buffers ---
    SDL_GPUBufferCreateInfo vertInfo = { SDL_GPU_BUFFERUSAGE_VERTEX, static_cast<Uint32>(sizeof(Vertex) * cubeVertices.size()) };
    SDL_GPUBufferCreateInfo indexInfo = { SDL_GPU_BUFFERUSAGE_INDEX, static_cast<Uint32>(sizeof(uint16_t) * cubeIndices.size()) };
    m_vertexBuffer = SDL_CreateGPUBuffer(m_gpuDevice, &vertInfo);
    m_indexBuffer = SDL_CreateGPUBuffer(m_gpuDevice, &indexInfo);

    SDL_GPUTransferBufferCreateInfo transferInfo = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, vertInfo.size + indexInfo.size };
    SDL_GPUTransferBuffer* staging = SDL_CreateGPUTransferBuffer(m_gpuDevice, &transferInfo);
    void* map = SDL_MapGPUTransferBuffer(m_gpuDevice, staging, false);
    std::memcpy(map, cubeVertices.data(), vertInfo.size);
    std::memcpy((Uint8*)map + vertInfo.size, cubeIndices.data(), indexInfo.size);
    SDL_UnmapGPUTransferBuffer(m_gpuDevice, staging);

    SDL_GPUCommandBuffer* copyCmd = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(copyCmd);
    SDL_GPUTransferBufferLocation srcV = { staging, 0 };
    SDL_GPUBufferRegion dstV = { m_vertexBuffer, 0, vertInfo.size };
    SDL_UploadToGPUBuffer(copyPass, &srcV, &dstV, false);
    SDL_GPUTransferBufferLocation srcI = { staging, vertInfo.size };
    SDL_GPUBufferRegion dstI = { m_indexBuffer, 0, indexInfo.size };
    SDL_UploadToGPUBuffer(copyPass, &srcI, &dstI, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(copyCmd);
    SDL_ReleaseGPUTransferBuffer(m_gpuDevice, staging);

        // --- SECTION 2: Generate "Welcome" Texture Surface via Font ---
    m_font = TTF_OpenFont("fonts/sans-serif.ttf", 48);
    if (!m_font) { std::cerr << "Font layout error! Keep sans-serif.ttf in the directory." << std::endl; return false; }
    
    SDL_Surface* textSurface = TTF_RenderText_Blended(m_font, "Welcome", 7, {255, 255, 255, 255});
    Uint32 texW = textSurface->w;
    Uint32 texH = textSurface->h;

    SDL_GPUTextureCreateInfo textureInfo = {};
    textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    textureInfo.width = texW;
    textureInfo.height = texH;
    textureInfo.layer_count_or_depth = 1;
    textureInfo.num_levels = 1;
    textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    m_textTexture = SDL_CreateGPUTexture(m_gpuDevice, &textureInfo);

    SDL_GPUTransferBufferCreateInfo texStagingInfo = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, static_cast<Uint32>(textSurface->pitch * texH) };
    SDL_GPUTransferBuffer* texStaging = SDL_CreateGPUTransferBuffer(m_gpuDevice, &texStagingInfo);
    void* texMap = SDL_MapGPUTransferBuffer(m_gpuDevice, texStaging, false);
    std::memcpy(texMap, textSurface->pixels, texStagingInfo.size);
    SDL_UnmapGPUTransferBuffer(m_gpuDevice, texStaging);

    copyCmd = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
    copyPass = SDL_BeginGPUCopyPass(copyCmd);
    SDL_GPUTextureTransferInfo srcTex = {};
    srcTex.transfer_buffer = texStaging;
    srcTex.offset = 0;
    srcTex.pixels_per_row = textSurface->pitch / 4;
    srcTex.rows_per_layer = texH;

    SDL_GPUTextureRegion dstTex = {};
    dstTex.texture = m_textTexture;
    dstTex.w = texW;
    dstTex.h = texH;
    dstTex.d = 1;

    SDL_UploadToGPUTexture(copyPass, &srcTex, &dstTex, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(copyCmd);
    SDL_ReleaseGPUTransferBuffer(m_gpuDevice, texStaging);
    SDL_DestroySurface(textSurface);

    // --- SECTION 2b: Edge-Clamped Sampler Configuration ---
    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    m_sampler = SDL_CreateGPUSampler(m_gpuDevice, &samplerInfo);

        // --- SECTION 3: Create 2D Quad Buffers for Billboard ---
    float wNorm = (float)texW / 800.0f;
    float hNorm = (float)texH / 600.0f;
    std::vector<TextureVertex> quadVerts = {
        {{-wNorm, -hNorm}, {0.0f, 1.0f}}, {{ wNorm, -hNorm}, {1.0f, 1.0f}},
        {{ wNorm,  hNorm}, {1.0f, 0.0f}}, {{-wNorm,  hNorm}, {0.0f, 0.0f}}
    };
    std::vector<uint16_t> quadIndices = { 0, 1, 2, 2, 3, 0 };

    SDL_GPUBufferCreateInfo qvInfo = { SDL_GPU_BUFFERUSAGE_VERTEX, static_cast<Uint32>(sizeof(TextureVertex) * quadVerts.size()) };
    SDL_GPUBufferCreateInfo qiInfo = { SDL_GPU_BUFFERUSAGE_INDEX, static_cast<Uint32>(sizeof(uint16_t) * quadIndices.size()) };
    m_quadVB = SDL_CreateGPUBuffer(m_gpuDevice, &qvInfo);
    m_quadIB = SDL_CreateGPUBuffer(m_gpuDevice, &qiInfo);

    transferInfo = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, qvInfo.size + qiInfo.size };
    staging = SDL_CreateGPUTransferBuffer(m_gpuDevice, &transferInfo);
    map = SDL_MapGPUTransferBuffer(m_gpuDevice, staging, false);
    std::memcpy(map, quadVerts.data(), qvInfo.size);
    std::memcpy((Uint8*)map + qvInfo.size, quadIndices.data(), qiInfo.size);
    SDL_UnmapGPUTransferBuffer(m_gpuDevice, staging);

    copyCmd = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
    copyPass = SDL_BeginGPUCopyPass(copyCmd);
    SDL_GPUTransferBufferLocation sQV = { staging, 0 };
    SDL_GPUBufferRegion dQV = { m_quadVB, 0, qvInfo.size };
    SDL_UploadToGPUBuffer(copyPass, &sQV, &dQV, false);
    SDL_GPUTransferBufferLocation sQI = { staging, qvInfo.size };
    SDL_GPUBufferRegion dQI = { m_quadIB, 0, qiInfo.size };
    SDL_UploadToGPUBuffer(copyPass, &sQI, &dQI, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(copyCmd);
    SDL_ReleaseGPUTransferBuffer(m_gpuDevice, staging);

    // --- SECTION 3b: Z Depth Buffer ---
    SDL_GPUTextureCreateInfo depthTextureInfo = {};
    depthTextureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    depthTextureInfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    depthTextureInfo.width = 800;
    depthTextureInfo.height = 600;
    depthTextureInfo.layer_count_or_depth = 1;
    depthTextureInfo.num_levels = 1;
    depthTextureInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    m_depthTexture = SDL_CreateGPUTexture(m_gpuDevice, &depthTextureInfo);

    // --- SECTION 4: Pipelines ---
    m_cubeVS = LoadShader(m_gpuDevice, "shaders/cube.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
    m_cubeFS = LoadShader(m_gpuDevice, "shaders/cube_frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 1);
    m_textVS = LoadShader(m_gpuDevice, "shaders/text.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
    m_textFS = LoadShader(m_gpuDevice, "shaders/text_frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);

    SDL_GPUVertexAttribute cubeAttrs[] = {
        { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0 },
        { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, sizeof(glm::vec3) }
    };
    SDL_GPUVertexBufferDescription cubeBufs[] = { { 0, sizeof(Vertex), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0 } };

    SDL_GPUTextureFormat targetFormat = SDL_GetGPUSwapchainTextureFormat(m_gpuDevice, m_window);
    SDL_GPUColorTargetDescription colorDesc = {};
    colorDesc.format = targetFormat;
    colorDesc.blend_state.enable_blend = true;
    colorDesc.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorDesc.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorDesc.blend_state.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    colorDesc.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    colorDesc.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    colorDesc.blend_state.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.vertex_shader = m_cubeVS;
    pipelineInfo.fragment_shader = m_cubeFS;
    pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
    pipelineInfo.vertex_input_state.vertex_attributes = cubeAttrs;
    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = cubeBufs;
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorDesc;
    pipelineInfo.depth_stencil_state.enable_depth_test = true;
    pipelineInfo.depth_stencil_state.enable_depth_write = true;
    pipelineInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    pipelineInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    pipelineInfo.target_info.has_depth_stencil_target = true;


    m_cubePipeline = SDL_CreateGPUGraphicsPipeline(m_gpuDevice, &pipelineInfo);
    SDL_GPUGraphicsPipelineCreateInfo textPipelineInfo = {};
    textPipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    textPipelineInfo.vertex_shader = m_textVS;
    textPipelineInfo.fragment_shader = m_textFS;
    SDL_GPUVertexAttribute textAttrs[] = {
        { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0 },
        { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(glm::vec2) }
    };
    SDL_GPUVertexBufferDescription textBufs[] = { { 0, sizeof(TextureVertex), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0 } };
    textPipelineInfo.vertex_input_state.num_vertex_attributes = 2;
    textPipelineInfo.vertex_input_state.vertex_attributes = textAttrs;
    textPipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    textPipelineInfo.vertex_input_state.vertex_buffer_descriptions = textBufs;
    textPipelineInfo.target_info.num_color_targets = 1;
    textPipelineInfo.target_info.color_target_descriptions = &colorDesc;
    textPipelineInfo.depth_stencil_state.enable_depth_test = false;
    textPipelineInfo.depth_stencil_state.enable_depth_write = false;
    textPipelineInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    textPipelineInfo.target_info.has_depth_stencil_target = true;

    m_textPipeline = SDL_CreateGPUGraphicsPipeline(m_gpuDevice, &textPipelineInfo);

    // --- SECTION 4c: Pre-Allocated Persistent Live FPS Assets ---
    SDL_GPUTextureCreateInfo fpsTexInfo = {};
    fpsTexInfo.type = SDL_GPU_TEXTURETYPE_2D;
    fpsTexInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    fpsTexInfo.width = m_fpsTexW;
    fpsTexInfo.height = m_fpsTexH;
    fpsTexInfo.layer_count_or_depth = 1;
    fpsTexInfo.num_levels = 1;
    fpsTexInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    m_fpsTexture = SDL_CreateGPUTexture(m_gpuDevice, &fpsTexInfo);

    // FIX: Compute the transfer allocation sizing inline (256 * 64 * 4 bytes per pixel = 65,536)
    texStagingInfo = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, m_fpsTexW * m_fpsTexH * 4 };
    m_fpsTexStaging = SDL_CreateGPUTransferBuffer(m_gpuDevice, &texStagingInfo);

    // Also verify that your geometry allocations use the accurate size calculations directly:
    SDL_GPUBufferCreateInfo fqvInfo = { SDL_GPU_BUFFERUSAGE_VERTEX, static_cast<Uint32>(sizeof(TextureVertex) * 4) };
    SDL_GPUBufferCreateInfo fqiInfo = { SDL_GPU_BUFFERUSAGE_INDEX, static_cast<Uint32>(sizeof(uint16_t) * 6) };
    m_fpsVB = SDL_CreateGPUBuffer(m_gpuDevice, &fqvInfo);
    m_fpsIB = SDL_CreateGPUBuffer(m_gpuDevice, &fqiInfo);

    SDL_GPUTransferBufferCreateInfo geomStagingInfo = { SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, fqvInfo.size + fqiInfo.size };
    m_fpsGeomStaging = SDL_CreateGPUTransferBuffer(m_gpuDevice, &geomStagingInfo);

    return true;
}

void Engine::UpdateFPSCounter(SDL_GPUCommandBuffer* cmd) {
    m_frameCount++;
    Uint64 currentTime = SDL_GetTicks();
    Uint64 elapsedTime = currentTime - m_lastFpsUpdateTime;
    if (elapsedTime >= 250) {
    m_currentFps = (m_frameCount * 1000.0f) / elapsedTime;
    m_frameCount = 0;
    m_lastFpsUpdateTime = currentTime;
    char fpsStr[32];
    SDL_snprintf(fpsStr, sizeof(fpsStr), "FPS: %.1f", m_currentFps);
    SDL_Color textColor = { 0, 255, 0, 255 };
    SDL_Color bgColor   = { 15, 30, 45, 60 }; // 180 out of 255 alpha gives a crisp semi-transparent backdrop!

    SDL_Surface* rawFpsSurface = TTF_RenderText_Shaded(m_font, fpsStr, 0, textColor, bgColor);
    if (rawFpsSurface) {
    SDL_Surface* fpsSurface = SDL_ConvertSurface(rawFpsSurface, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(rawFpsSurface);
    if (fpsSurface) {
    void* fpsMap = SDL_MapGPUTransferBuffer(m_gpuDevice, m_fpsTexStaging, false);
    std::memset(fpsMap, 0, m_fpsTexW * m_fpsTexH * 4);
    Uint8* dstPixels = (Uint8*)fpsMap;
    Uint8* srcPixels = (Uint8*)fpsSurface->pixels;
    Uint32 copyW = std::min(static_cast<Uint32>(fpsSurface->w), m_fpsTexW);
    Uint32 copyH = std::min(static_cast<Uint32>(fpsSurface->h), m_fpsTexH);
    for (Uint32 y = 0; y < copyH; ++y) {
    std::memcpy(dstPixels + (y * m_fpsTexW * 4), srcPixels + (y * fpsSurface->pitch), copyW * 4);
    }
    SDL_UnmapGPUTransferBuffer(m_gpuDevice, m_fpsTexStaging);
    SDL_GPUCopyPass* fpsCopyPass = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureTransferInfo srcFpsTex = {};
    srcFpsTex.transfer_buffer = m_fpsTexStaging;
    srcFpsTex.offset          = 0;
    srcFpsTex.pixels_per_row  = m_fpsTexW;
    srcFpsTex.rows_per_layer  = m_fpsTexH;
    SDL_GPUTextureRegion dstFpsTex = {};
    dstFpsTex.texture   = m_fpsTexture;
    dstFpsTex.w         = m_fpsTexW;
    dstFpsTex.h         = m_fpsTexH;
    dstFpsTex.d         = 1;
    SDL_UploadToGPUTexture(fpsCopyPass, &srcFpsTex, &dstFpsTex, false);
    float maxU = static_cast<float>(fpsSurface->w) / static_cast<float>(m_fpsTexW);
    float maxV = static_cast<float>(fpsSurface->h) / static_cast<float>(m_fpsTexH);
    float textScale = 0.4f;
    float wNormFPS = (float)fpsSurface->w / 800.0f * textScale;
    float hNormFPS = (float)fpsSurface->h / 600.0f * textScale;
    float rightEdge = (1.0f - 0.02f) - (100.0f / 800.0f);
    float topEdge   = (1.0f - 0.02f) - (100.0f / 600.0f);
    std::vector<TextureVertex> fpsQuadVerts = {
    {{rightEdge - (wNormFPS * 2.0f), topEdge},                     {0.0f, 0.0f}},
    {{rightEdge,                     topEdge},                     {maxU, 0.0f}},
    {{rightEdge,                     topEdge - (hNormFPS * 2.0f)}, {maxU, maxV}},
    {{rightEdge - (wNormFPS * 2.0f), topEdge - (hNormFPS * 2.0f)}, {0.0f, maxV}}
    };
    std::vector<uint16_t> fpsQuadIndices = { 0, 1, 2, 2, 3, 0 };
 // Map updated geometry data to geometry persistent upload buffer
void* geomMap = SDL_MapGPUTransferBuffer(m_gpuDevice, m_fpsGeomStaging, false);
std::memcpy(geomMap, fpsQuadVerts.data(), sizeof(TextureVertex) * 4);
std::memcpy((Uint8*)geomMap + (sizeof(TextureVertex) * 4), fpsQuadIndices.data(), sizeof(uint16_t) * 6);
SDL_UnmapGPUTransferBuffer(m_gpuDevice, m_fpsGeomStaging);

SDL_GPUTransferBufferLocation sFQV = { m_fpsGeomStaging, 0 };
// FIX: Replace fqvInfo.size with an explicit sizeof calculation inline
SDL_GPUBufferRegion dFQV = { m_fpsVB, 0, static_cast<Uint32>(sizeof(TextureVertex) * 4) };
SDL_UploadToGPUBuffer(fpsCopyPass, &sFQV, &dFQV, false);

// FIX: Replace fqvInfo.size and fqiInfo.size with exact structural values
SDL_GPUTransferBufferLocation sFQI = { m_fpsGeomStaging, static_cast<Uint32>(sizeof(TextureVertex) * 4) };
SDL_GPUBufferRegion dFQI = { m_fpsIB, 0, static_cast<Uint32>(sizeof(uint16_t) * 6) };
SDL_UploadToGPUBuffer(fpsCopyPass, &sFQI, &dFQI, false);

    SDL_EndGPUCopyPass(fpsCopyPass);
    SDL_DestroySurface(fpsSurface); // Clean up converted allocation
    m_hasFpsData = true;
    }
    }
    }
}

void Engine::Run() {
    m_running = true;
    SDL_Event event;
    while (m_running) {
        // Calculate smooth delta time step (seconds elapsed since last frame)
        Uint64 currentFrameTime = SDL_GetTicks();
        float deltaTime = static_cast<float>(currentFrameTime - m_lastFrameTime) / 1000.0f;
        m_lastFrameTime = currentFrameTime;

        // Process events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) m_running = false;
            
            // Forward events down to your input handler module
            m_inputHandler.HandleEvent(event);
        }

        // Apply movement updates
        m_inputHandler.Update(deltaTime);
        m_rotation += 0.015f;
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
    if (!cmd) continue;
    UpdateFPSCounter(cmd);
    RenderFrame(cmd);
    SDL_SubmitGPUCommandBuffer(cmd);
    }
}

void Engine::RenderFrame(SDL_GPUCommandBuffer* cmd) {
    // 1. Grab inputs position offset modifications
    const glm::vec3& cubeOffset = m_inputHandler.GetPositionOffset();

    glm::mat4 model = glm::translate(glm::mat4(1.0f), cubeOffset);
    model = glm::rotate(model, m_rotation, glm::vec3(0.5f, 1.0f, 0.0f));

    glm::mat4 view  = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.2f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj  = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 10.0f);
    
    // 2. Fetch fog environment configurations
    const FogUniforms& fogSettings = m_effects.GetFogSettings();

    // 3. Assemble our unified data frame block
    FrameUniforms frameData;
    frameData.mvp = proj * view * model;
    frameData.modelView = view * model;
    frameData.fogColor = fogSettings.color;
    frameData.fogStart = 2.0f; // Hardcoded safe start boundary for testing
    frameData.fogEnd = 2.2f;   // Completely hidden at 6.0 units away
    frameData.packingPadding = glm::vec2(0.0f, 0.0f);

    Uint32 w, h;
    SDL_GPUTexture* swapchainTex;
    bool acquired = SDL_AcquireGPUSwapchainTexture(cmd, m_window, &swapchainTex, &w, &h);
    
    if (acquired && swapchainTex) {
        SDL_GPUColorTargetInfo colorTarget = {};
        colorTarget.texture = swapchainTex;
        colorTarget.clear_color = { 0.1f, 0.1f, 0.15f, 1.0f }; // Matches fogColor exactly
        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPUDepthStencilTargetInfo depthTarget = {};
        depthTarget.texture = m_depthTexture;
        depthTarget.clear_depth = 1.0f;
        depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;

        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, &depthTarget);
        
        // Pass 1: Solid Spinning 3D Cube
        SDL_BindGPUGraphicsPipeline(renderPass, m_cubePipeline);
        
        // CRITICAL STEP: Push the EXACT same uniform block structure to both shader stages!
        SDL_PushGPUVertexUniformData(cmd, 0, &frameData, sizeof(FrameUniforms));
        SDL_PushGPUFragmentUniformData(cmd, 0, &frameData, sizeof(FrameUniforms));

        SDL_GPUBufferBinding cVB = { m_vertexBuffer, 0 };
        SDL_BindGPUVertexBuffers(renderPass, 0, &cVB, 1);
        SDL_GPUBufferBinding cIB = { m_indexBuffer, 0 };
        SDL_BindGPUIndexBuffer(renderPass, &cIB, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_DrawGPUIndexedPrimitives(renderPass, 36, 1, 0, 0, 0);
    // Pass 2: Centered right-side-up "Welcome" message
    SDL_BindGPUGraphicsPipeline(renderPass, m_textPipeline);
    SDL_GPUTextureSamplerBinding samplerBinding = { m_textTexture, m_sampler };
    SDL_BindGPUFragmentSamplers(renderPass, 0, &samplerBinding, 1);
    SDL_GPUBufferBinding tVB = { m_quadVB, 0 };
    SDL_BindGPUVertexBuffers(renderPass, 0, &tVB, 1);
    SDL_GPUBufferBinding tIB = { m_quadIB, 0 };
    SDL_BindGPUIndexBuffer(renderPass, &tIB, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
    // Pass 3: Live FPS Ticker Overlay
    if (m_hasFpsData) {
    SDL_GPUTextureSamplerBinding fpsSamplerBinding = { m_fpsTexture, m_sampler };
    SDL_BindGPUFragmentSamplers(renderPass, 0, &fpsSamplerBinding, 1);
    SDL_GPUBufferBinding fVB = { m_fpsVB, 0 };
    SDL_BindGPUVertexBuffers(renderPass, 0, &fVB, 1);
    SDL_GPUBufferBinding fIB = { m_fpsIB, 0 };
    SDL_BindGPUIndexBuffer(renderPass, &fIB, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
    }
    SDL_EndGPURenderPass(renderPass);
    }
}

void Engine::Cleanup() {
    TTF_CloseFont(m_font);
    SDL_ReleaseGPUSampler(m_gpuDevice, m_sampler);
    SDL_ReleaseGPUTexture(m_gpuDevice, m_textTexture);
    SDL_ReleaseGPUTexture(m_gpuDevice, m_depthTexture);
    if (m_fpsTexture) SDL_ReleaseGPUTexture(m_gpuDevice, m_fpsTexture);
    SDL_ReleaseGPUTransferBuffer(m_gpuDevice, m_fpsTexStaging);
    SDL_ReleaseGPUTransferBuffer(m_gpuDevice, m_fpsGeomStaging);
    SDL_ReleaseGPUBuffer(m_gpuDevice, m_vertexBuffer);
    SDL_ReleaseGPUBuffer(m_gpuDevice, m_indexBuffer);
    SDL_ReleaseGPUBuffer(m_gpuDevice, m_quadVB);
    SDL_ReleaseGPUBuffer(m_gpuDevice, m_quadIB);
    if (m_fpsVB) SDL_ReleaseGPUBuffer(m_gpuDevice, m_fpsVB);
    if (m_fpsIB) SDL_ReleaseGPUBuffer(m_gpuDevice, m_fpsIB);
    if (m_cubePipeline) SDL_ReleaseGPUGraphicsPipeline(m_gpuDevice, m_cubePipeline);
    if (m_textPipeline) SDL_ReleaseGPUGraphicsPipeline(m_gpuDevice, m_textPipeline);
    if (m_cubeVS) SDL_ReleaseGPUShader(m_gpuDevice, m_cubeVS);
    if (m_cubeFS) SDL_ReleaseGPUShader(m_gpuDevice, m_cubeFS);
    if (m_textVS) SDL_ReleaseGPUShader(m_gpuDevice, m_textVS);
    if (m_textFS) SDL_ReleaseGPUShader(m_gpuDevice, m_textFS);
    SDL_WaitForGPUIdle(m_gpuDevice);
    if (m_gpuDevice) SDL_DestroyGPUDevice(m_gpuDevice);
    SDL_DestroyWindow(m_window);
    TTF_Quit();
    SDL_Quit();
}
