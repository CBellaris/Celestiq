#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <memory>

// 面光：四个顶点、颜色、强度
struct alignas(16) FaceLightData {
    alignas(16) glm::vec3 v0; // 顺序：逆时针
    alignas(16) glm::vec3 v1;
    alignas(16) glm::vec3 v2;
    alignas(16) glm::vec3 v3;
    alignas(16) glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float cdf = 0.0f;

    FaceLightData() {
        // 单位矩形，逆时针：左下，右下，右上，左上
        v0 = glm::vec3(-1.0f, -1.0f, 0.0f);
        v1 = glm::vec3( 1.0f, -1.0f, 0.0f);
        v2 = glm::vec3( 1.0f,  1.0f, 0.0f);
        v3 = glm::vec3(-1.0f,  1.0f, 0.0f);
    }
};

// 方向光：方向、颜色、强度
struct alignas(16) DirectionalLightData {
    alignas(16) glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f); // 单位向量
    alignas(16) glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float angularRadius = 0.04f;
};

// 提供单个光源管理方法，不支持外部创建，仅由Lights类创建
class FaceLight {
    friend class Lights;
    uint32_t id;
    FaceLightData data;
    FaceLight(uint32_t id_, const FaceLightData& d) : id(id_), data(d) {}

    // 变换参数
    glm::vec3 position = glm::vec3(0.0f);
    float rotation = 0.0f;              // 弧度，绕Z轴
    glm::vec2 scale = glm::vec2(1.0f);  // XY缩放

    void updateVertices() {
        static const glm::vec3 defaultVerts[4] = {
            // glm::vec3(-1.0f, -1.0f, 0.0f),
            // glm::vec3( 1.0f, -1.0f, 0.0f),
            // glm::vec3( 1.0f,  1.0f, 0.0f),
            // glm::vec3(-1.0f,  1.0f, 0.0f),
            glm::vec3(-1.0f, 0.0f, -1.0f),
            glm::vec3( 1.0f, 0.0f, -1.0f),
            glm::vec3( 1.0f, 0.0f,  1.0f),
            glm::vec3(-1.0f, 0.0f,  1.0f),
        };

        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, position);
        transform = glm::rotate(transform, rotation, glm::vec3(0, 0, 1));
        transform = glm::scale(transform, glm::vec3(scale, 1.0f));

        glm::vec3 transformedVerts[4];
        for (int i = 0; i < 4; ++i) {
            glm::vec4 transformed = transform * glm::vec4(defaultVerts[i], 1.0f);
            transformedVerts[i] = glm::vec3(transformed);
        }
        data.v0 = transformedVerts[0];
        data.v1 = transformedVerts[1];
        data.v2 = transformedVerts[2];
        data.v3 = transformedVerts[3];
    }
public:
    const FaceLightData& getData() const { return data; }
    FaceLightData& getMutableData() { return data; }  // 可修改版本
    uint32_t getId() const { return id; }

    
    // 接口：设置变换
    void setPosition(const glm::vec3& pos) {
        position = pos;
        updateVertices();
    }

    void setRotation(float radians) {
        rotation = radians;
        updateVertices();
    }

    void setScale(const glm::vec2& s) {
        scale = s;
        updateVertices();
    }

    // getter
    glm::vec3 getPosition() const { return position; }
    float getRotation() const { return rotation; }
    glm::vec2 getScale() const { return scale; }
};

// 提供单个光源管理方法，不支持外部创建，仅由Lights类创建
class DirectionalLight {
    friend class Lights;
    uint32_t id;
    DirectionalLightData data;
    DirectionalLight(uint32_t id_, const DirectionalLightData& d) : id(id_), data(d) {}
public:
    const DirectionalLightData& getData() const { return data; }
    DirectionalLightData& getMutableData() { return data; }
    uint32_t getId() const { return id; }
};

// 管理全局所有灯光，提供创建单个灯光（分配id）、id索引对象、返回所有面光信息以创建SSBO等方法
class Lights {
private:
    Lights() = default;
    ~Lights() {clear();}
    std::vector<FaceLight> faceLights;
    std::vector<DirectionalLight> directionalLights;

    std::vector<float> faceLightPowerCDF; // 面光功率

    void accumulateFaceLightPower(){
        faceLightPowerCDF.clear();
        faceLightPowerCDF.reserve(faceLights.size());
        float totalPower = 0.0f;
        for (const auto& light : faceLights) {
            float area = glm::length(glm::cross(light.getData().v1 - light.getData().v0, light.getData().v3 - light.getData().v0));
            float power = light.getData().intensity * area;
            totalPower += power;
            faceLightPowerCDF.push_back(totalPower);
        }
        for (auto& power : faceLightPowerCDF) {
            power /= totalPower;
        }
    }

public:
    static Lights& get() {
        static Lights inst;
        return inst;
    }

    uint32_t createFaceLight(const FaceLightData& data) {
        uint32_t id = static_cast<uint32_t>(faceLights.size());
        faceLights.push_back(FaceLight(id, data));
        accumulateFaceLightPower();
        for (auto& light : faceLights) {
            light.getMutableData().cdf = faceLightPowerCDF[light.getId()];
        }
        return id;
    }

    uint32_t createDirectionalLight(const DirectionalLightData& data) {
        uint32_t id = static_cast<uint32_t>(directionalLights.size());
        directionalLights.push_back(DirectionalLight(id, data));
        return id;
    }

    FaceLight& getFaceLight(uint32_t id) {
        assert(id < faceLights.size());
        return faceLights[id];
    }

    DirectionalLight& getDirectionalLight(uint32_t id) {
        assert(id < directionalLights.size());
        return directionalLights[id];
    }

    const std::vector<FaceLight>& getAllFaceLights() const {
        return faceLights;
    }

    const std::vector<DirectionalLight>& getAllDirectionalLights() const {
        return directionalLights;
    }

    // 返回所需SSBO大小
    uint64_t getFaceLightDataSize() {
        return faceLights.size() * sizeof(FaceLightData);
    }

    uint64_t getDirectionalLightDataSize() {
        return directionalLights.size() * sizeof(DirectionalLightData);
    }

    // 将数据打包为GPU上传格式
    std::vector<FaceLightData> getFaceLightDataBuffer() const {
        std::vector<FaceLightData> result;
        result.reserve(faceLights.size());
        for (const auto& l : faceLights) result.push_back(l.getData());
        return result;
    }

    std::vector<DirectionalLightData> getDirectionalLightDataBuffer() const {
        std::vector<DirectionalLightData> result;
        result.reserve(directionalLights.size());
        for (const auto& l : directionalLights) result.push_back(l.getData());
        return result;
    }

    void clear() {
        faceLights.clear();
        directionalLights.clear();
    }
};

