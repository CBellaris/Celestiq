#include "Camera.h"

Camera::Camera() : 
    pos(glm::vec3(0.0f, 0.0f, 3.0f)), 
    target(glm::vec3(0.0f)), 
    up(glm::vec3(0.0f, 1.0f, 0.0f)), 
    lockTarget(false),
    fov(45.0f),
    aspectRatio(16.0f / 9.0f),
    nearPlane(0.1f),
    farPlane(30.0f),
    orthographic(false),
    cameraSpeed(0.8f),
    cameraSensitivity(0.3f)
{
    cameraUBO.Create(sizeof(CameraUnit));

    direction = glm::normalize(target - pos);
    cameraRight = glm::normalize(glm::cross(direction, up));
    cameraUp = glm::normalize(glm::cross(cameraRight, direction)); // 重新计算上向量
    cameraUnit.cameraPosition = pos;
    updateViewMatrix();
    updateProjectionMatrix();
}

Camera::~Camera()
{
    delete cameraUBO;
}

void Camera::setCameraPosition(const glm::vec3& newPos)
{
    pos = newPos;
    cameraUnit.cameraPosition = pos;
    if (lockTarget)
    {
        direction = glm::normalize(target - pos);

    }
    updateCameraVectors();
}

void Camera::setCameraDirection(const glm::vec3& newDirection)
{
    lockTarget = false;
    direction = glm::normalize(newDirection);
    updateCameraVectors();
}

void Camera::setCameraLookAt(const std::optional<glm::vec3>& newTarget)
{
    if (!newTarget.has_value())
    {
        // 取消锁定
        lockTarget = false;
    }
    else
    {
        // 锁定到新目标
        target = newTarget.value();
        lockTarget = true;
        direction = glm::normalize(target - pos);
        updateCameraVectors();
    }
}

void Camera::setPerspective(float newFov, float newAspectRatio, float newNearPlane, float newFarPlane)
{
    fov = newFov;
    aspectRatio = newAspectRatio;
    nearPlane = newNearPlane;
    farPlane = newFarPlane;
    orthographic = false;
    updateProjectionMatrix();
}

void Camera::setOrthographic(float left, float right, float bottom, float top, float newNearPlane, float newFarPlane)
{
    nearPlane = newNearPlane;
    farPlane = newFarPlane;
    orthographic = true;
    projectionMatrix = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
}

void Camera::setProjectionMode(bool useOrthographic)
{
    orthographic = useOrthographic;
    updateProjectionMatrix();
}

void Camera::processKey(bool Press_W, bool Press_A, bool Press_S, bool Press_D, float deltaTime)
{
    if (Press_W){
        setCameraPosition(pos + cameraSpeed * deltaTime * direction);
        updateViewMatrix();
    }
    if (Press_A){
        setCameraPosition(pos - cameraSpeed * deltaTime * cameraRight);
        updateViewMatrix();
    }
    if (Press_S){
        setCameraPosition(pos - cameraSpeed * deltaTime * direction);
        updateViewMatrix();
    }
    if (Press_D){
        setCameraPosition(pos + cameraSpeed * deltaTime * cameraRight);
        updateViewMatrix();
    }

}

void Camera::processMouseMove(float xPos, float yPos)
{
    if(firstMouse)
    {
        lastX = xPos;
        lastY = yPos;
        pitch = glm::degrees(asin(direction.y));
        yaw = glm::degrees(atan2(direction.z, direction.x));
        firstMouse = false;
    }
    else
    {
        float xoffset = xPos - lastX;
        float yoffset = lastY - yPos; 
        if(abs(xoffset) < 1e-5 && abs(yoffset) < 1e-5)
            return;
        lastX = xPos;
        lastY = yPos;

        xoffset *= cameraSensitivity;
        yoffset *= cameraSensitivity;

        yaw   += xoffset;
        pitch += yoffset;

        if(pitch > 89.0f)
            pitch = 89.0f;
        if(pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        setCameraDirection(front);
    }
}

void Camera::updateViewMatrix()
{
    // 使用 pos、direction、cameraUp 计算视图矩阵
    viewMatrix = glm::lookAt(pos, pos + direction, cameraUp);
    updataViewProjectionMatrix();
}

void Camera::updateCameraVectors()
{
    // 更新摄像机的方向和上轴
    cameraRight = glm::normalize(glm::cross(direction, up));
    cameraUp = glm::normalize(glm::cross(cameraRight, direction)); 
    updateViewMatrix();
}

void Camera::updateProjectionMatrix()
{
    if (orthographic)
    {
        // Update orthographic projection (default bounds for simplicity)
        projectionMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, nearPlane, farPlane);
    }
    else
    {
        // Update perspective projection
        projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);

    }
    updataViewProjectionMatrix();
}

void Camera::updataViewProjectionMatrix()
{
    cameraUpdate();
    viewProjectionMatrix = projectionMatrix * viewMatrix;
    cameraUnit.viewProjectionMatrix = viewProjectionMatrix;
    cameraUBO.TransferData(&cameraUnit, sizeof(CameraUnit));
}