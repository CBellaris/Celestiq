#version 460
#pragma shader_stage(fragment)

layout(location = 0) out vec4 o_Color;

void main() {
    o_Color = vec4(0.56, 0.83, 0.93, 1);
}