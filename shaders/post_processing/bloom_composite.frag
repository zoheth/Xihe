#version 450
layout(location = 0) out vec4 o_color;
layout(location = 0) in vec2 in_uv;
layout(set = 0, binding = 0) uniform sampler2D hdr_tex;
layout(set = 0, binding = 1) uniform sampler2D bloom_tex; 
layout(push_constant) uniform Registers {
    float bloom_strength;    
    float exposure;         
} registers;

vec3 reinhard_extended(vec3 hdr_color, float max_white) {
    vec3 numerator = hdr_color * (1.0 + (hdr_color / (max_white * max_white)));
    return numerator / (1.0 + hdr_color);
}

void main() {
    vec3 hdr = textureLod(hdr_tex, in_uv, 0.0).rgb * registers.exposure;
    vec3 bloom = textureLod(bloom_tex, in_uv, 0.0).rgb * registers.exposure;
    
    vec3 combined = hdr + bloom * registers.bloom_strength;
    
    vec3 tone_mapped = reinhard_extended(combined, 4.0);
    
    o_color = vec4(tone_mapped, 1.0);
}