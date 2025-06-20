#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>
#include <vector>

#include "VKBase.h"

struct CameraUnit
{
    glm::mat4 viewProjectionMatrix; // 视图投影矩阵
    alignas(16)glm::vec3 cameraPosition;       // 摄像机位置

    CameraUnit() : viewProjectionMatrix(1.0f), cameraPosition(1.0f) {}
};

class Camera
{
private:
    // 摄像机外属性
    glm::vec3 pos;
    glm::vec3 target;
    glm::vec3 direction;
    glm::vec3 up;
    glm::vec3 cameraRight;
    glm::vec3 cameraUp;
    bool lockTarget; 
    glm::mat4 viewMatrix; 
    
    // 摄像机内属性
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    bool orthographic;

    // 投影矩阵
    glm::mat4 projectionMatrix;

    // 最终的摄像机变换矩阵
    glm::mat4 viewProjectionMatrix;

    // 摄像机移动速度和鼠标灵敏度
    float cameraSpeed;
    float cameraSensitivity;

    bool firstMouse = true;
    float lastX = 0.0f;
    float lastY = 0.0f;
    float yaw = -90.0f; // 偏航角
    float pitch = 0.0f; // 俯仰角

    // UBO
    CameraUnit cameraUnit;
    Celestiq::Vulkan::uniformBuffer cameraUBO;

    std::vector<std::function<void()>> cameraUpdateCallbacks; // 摄像机更新回调函数列表

public:
    Camera();
    ~Camera();

    // 设置属性
    void setCameraPosition(const glm::vec3& pos);
    void setCameraDirection(const glm::vec3& direction);
    void setCameraLookAt(const std::optional<glm::vec3>& newTarget);

    void setPerspective(float fov, float aspectRatio, float nearPlane, float farPlane);
    void setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
    void setProjectionMode(bool useOrthographic);

    // 一些访问接口
    inline const glm::mat4& getViewMatrix() const {return viewMatrix;}
    inline const glm::mat4& getViewProjectionMatrix() const {return viewProjectionMatrix;}

    inline const float getCameraSensitivity() const {return cameraSensitivity;}
    inline const glm::vec3 getCameradirection() const {return direction;}

    inline const VkBuffer getCameraUBO() const {return cameraUBO;}
    inline const VkDeviceSize getCameraUBOSize() const {return sizeof(CameraUnit);}
    inline void resetFirstMouse() {firstMouse = true;}

    // 处理按键
    void processKey(bool Press_W, bool Press_A, bool Press_S, bool Press_D, float deltaTime);
    // 处理鼠标移动
    void processMouseMove(float xPos, float yPos);
    // 摄像机更新回调
    void addCameraUpdateCallback(std::function<void()> callback) {
        cameraUpdateCallbacks.push_back(callback);
    }
    void cameraUpdate() {
        for (const auto& callback : cameraUpdateCallbacks) {
            callback();
        }
    }
    
private:
    void updateViewMatrix();
    void updateCameraVectors();
    void updateProjectionMatrix();
    void updataViewProjectionMatrix();
};