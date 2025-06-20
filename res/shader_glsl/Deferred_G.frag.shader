#version 460
#pragma shader_stage(fragment)

layout(location = 0) out vec4 o_Position;

layout(location = 0) in vec3 fragPosition; // 片段位置

void main() {
    vec3 fragPosition_map = fragPosition * 0.5 + 0.5; 
    o_Position = vec4(fragPosition_map, 1.0);
}