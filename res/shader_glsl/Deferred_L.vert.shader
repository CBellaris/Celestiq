#version 450
#pragma shader_stage(vertex)

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in float inTangentW;

layout(location = 0) out vec2 fragTexCoords; // 片段纹理坐标

void main() {
    fragTexCoords = inTexCoords; // 传递纹理坐标到片段着色器
    gl_Position = vec4(inPosition, 1.0);
}