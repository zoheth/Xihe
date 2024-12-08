#version 450
layout(location = 0) out vec4 o_color;
layout(location = 0) in vec2 in_uv;
layout(set = 0, binding = 0) uniform sampler2D hdr_tex;
layout(set = 0, binding = 1) uniform sampler2D bloom_tex; 

layout(push_constant) uniform Registers {
    float bloom_strength;    
    float exposure;         
} registers;

vec3 reinhard_tone_mapping(vec3 hdr_color) {
    return hdr_color / (vec3(1.0) + hdr_color);
}

vec3 sharpen(sampler2D tex, vec2 uv, float strength) {
    vec2 texelSize = 1.0 / textureSize(tex, 0);
    
    vec3 center = textureLod(tex, uv, 0.0).rgb;
    vec3 blur = textureLod(tex, uv + vec2(texelSize.x, 0.0), 0.0).rgb +
                textureLod(tex, uv - vec2(texelSize.x, 0.0), 0.0).rgb +
                textureLod(tex, uv + vec2(0.0, texelSize.y), 0.0).rgb +
                textureLod(tex, uv - vec2(0.0, texelSize.y), 0.0).rgb;
    blur *= 0.25;
    
    return center + (center - blur) * strength;
}

void main() {
    vec3 hdr = textureLod(hdr_tex, in_uv, 0.0).rgb;
    
    // 对bloom进行适度锐化
    vec3 bloom = sharpen(bloom_tex, in_uv, 0.5);

    vec3 combined = hdr + bloom * registers.bloom_strength;

    combined *= registers.exposure;
    
    vec3 tone_mapped = reinhard_tone_mapping(combined);
    
    o_color = vec4(tone_mapped, 1.0);
}