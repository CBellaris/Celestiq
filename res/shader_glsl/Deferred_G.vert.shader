#version 450
#pragma shader_stage(vertex)

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in float inTangentW;

layout(binding = 0) uniform Camera {
    mat4 viewProjectionMatrix; // 视图投影矩阵
    vec3 cameraPosition;       // 摄像机位置
};

layout(location = 0) out vec3 fragPosition; // 片段位置

void main() {
    fragPosition = inPosition; // 传递顶点位置到片段着色器
    gl_Position = viewProjectionMatrix * vec4(inPosition, 1.0);
}