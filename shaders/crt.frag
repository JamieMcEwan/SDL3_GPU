#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D offscreenTexture;

// Curvature distortion factor (higher numbers = more curved)
const float barrelDistortion = 0.15;

vec2 CurveScreen(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    uv.x *= 1.0 + (uv.y * uv.y) * barrelDistortion;
    uv.y *= 1.0 + (uv.x * uv.x) * barrelDistortion;
    uv = uv * 0.5 + 0.5;
    return uv;
}

void main() {
    // 1. Apply spherical screen curvature warping
    vec2 curvedUV = CurveScreen(inUV);

    // 2. Hard clip pixels outside the screen bounds to create black screen borders
    if (curvedUV.x < 0.0 || curvedUV.x > 1.0 || curvedUV.y < 0.0 || curvedUV.y > 1.0) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // 3. Sample the clean offscreen buffer
    vec4 baseColor = texture(offscreenTexture, curvedUV);

    // 4. Generate retro scanlines using a vertical sine wave
    // Multiplying by 600.0 maps the waves roughly to your window height resolution
    float scanline = sin(curvedUV.y * 600.0 * 2.0) * 0.12;
    vec3 crtColor = baseColor.rgb - scanline;

    // 5. Add a vignette shadow to darken the corners of the glass monitor
    float vignette = curvedUV.x * curvedUV.y * (1.0 - curvedUV.x) * (1.0 - curvedUV.y);
    vignette = clamp(pow(16.0 * vignette, 0.25), 0.0, 1.0);
    
    outColor = vec4(crtColor * vignette, baseColor.a);
}
