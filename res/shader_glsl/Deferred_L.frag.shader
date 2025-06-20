#version 460
#pragma shader_stage(fragment)

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec2 uv; // 片段纹理坐标

layout(binding = 0) uniform sampler2D computeImage;

void main() {
    vec3 col = texture(computeImage, uv).xyz;
    col = pow(col, vec3(1.0 / 2.2)); // gamma校正
    o_Color = vec4(col, 1.0);
}