/*


We are continuing a C++ SDL3 GPU Vulkan project with a 3D rotating cube (View-space linear depth fog), center text overlay ("Welcome"), and a scaled green live FPS counter in the top-right corner with a semi-transparent background box. 

The project compiles cleanly and runs with zero leaks or validation errors. We just finished converting it into a Render-to-Texture (RTT) architecture to implement a full-screen retro CRT post-processing filter.

Here is the current architectural state of our codebase:

1. `Types.hpp`
----------------------------------
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

struct FrameUniforms {
    glm::mat4 mvp;
    glm::mat4 modelView;
    glm::vec4 fogColor;
    float fogStart;
    float fogEnd;
    glm::vec2 padding;
};

extern const std::vector<Vertex> cubeVertices;
extern const std::vector<uint16_t> cubeIndices;

2. `ShaderLoader.hpp` & .cpp
----------------------------------
#pragma once
#include <SDL3/SDL.h>
SDL_GPUShader* LoadShader(SDL_GPUDevice* device, const char* filepath, SDL_GPUShaderStage stage, Uint32 numSamplers, Uint32 numUniformBuffers);

3. Shaders State
----------------------------------
- `cube.vert`: Reads matrix and position data from set=1, binding=0. Outputs color and view-space fragPos (location=1).
- `cube.frag`: Reads fog data from set=3, binding=0. Applies linear depth fog calculation using -fragPos.z.
- `crt.vert`: Generates a procedural full-screen triangle using gl_VertexIndex. Outputs outUV.
- `crt.frag`: Distorts UVs for barrel screen curvature, samples set=0, binding=0, applies sine-wave scanlines and vignette.

4. Current Goal
----------------------------------
We have just updated Engine.hpp, Engine.cpp, and our rendering passes to isolate the scene drawing inside a private offscreen texture canvas, which is then fed into our new CRT post-processing pipeline right before submission. 

Let's pick up right where we left off to check compilation status, tweak the retro aesthetic, or expand the engine features. Please acknowledge our project status!




*/











#include "engine.h"

int main(int argc, char* argv[]) {
    Engine app;
    if (!app.Initialize()) {
        return -1;
    }
    app.Run();
    return 0;
}
