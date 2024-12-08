#version 450
layout(location = 0) out vec4 o_color;
layout(location = 0) in vec2 in_uv;
layout(set = 0, binding = 0) uniform sampler2D hdr_tex;
layout(set = 0, binding = 1) uniform sampler2D bloom_tex; 

layout(push_constant) uniform Registers {
    float bloom_strength;    
    float exposure;         
} registers;

// Improved tone mapping with shoulder strength control
vec3 improved_reinhard(vec3 hdr_color, float shoulder_strength) {
    vec3 numerator = hdr_color * (1.0 + (hdr_color / vec3(shoulder_strength * shoulder_strength)));
    return numerator / (1.0 + hdr_color);
}

void main() {
    // Sample with bilinear filtering for both textures
    vec2 texelSize = 1.0 / textureSize(hdr_tex, 0);
    
    // Apply 2x2 box filter for HDR texture
    vec3 hdr = vec3(0.0);
    hdr += textureOffset(hdr_tex, in_uv, ivec2(-1, -1)).rgb * 0.25;
    hdr += textureOffset(hdr_tex, in_uv, ivec2(1, -1)).rgb * 0.25;
    hdr += textureOffset(hdr_tex, in_uv, ivec2(-1, 1)).rgb * 0.25;
    hdr += textureOffset(hdr_tex, in_uv, ivec2(1, 1)).rgb * 0.25;
    
    // Sample bloom with smooth bilinear filtering
    vec3 bloom = textureLod(bloom_tex, in_uv, 0.0).rgb;
    
    // Combine with smooth interpolation
    vec3 combined = mix(hdr, hdr + bloom, smoothstep(0.0, 1.0, registers.bloom_strength));
    
    // Apply exposure with smoothing
    combined *= max(0.0, registers.exposure);
    
    // Improved tone mapping
    vec3 tone_mapped = improved_reinhard(combined, 0.8);
    
    o_color = vec4(tone_mapped, 1.0);
}