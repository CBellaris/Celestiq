#pragma once
#include "Mesh.h"
#include "Material.h"
#include "Light.h"
#include "Camera.h"
#include "VKBase.h"
#include "Input.h"

#include <glm/glm.hpp>

#include <vector>
#include <memory>

using namespace Celestiq::Vulkan;

// 轴对齐包围盒
struct alignas(16) AABB {
    alignas(16) glm::vec3 min = glm::vec3(FLT_MAX);
    alignas(16) glm::vec3 max = glm::vec3(-FLT_MAX);

    void expand(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    void merge(const AABB& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
    }

    glm::vec3 center() const { return 0.5f * (min + max); }
};

// BLAS 节点结构（适合 GPU 上传）
struct alignas(16) BLASNode {
    AABB bounds;
    glm::ivec3 indices = glm::ivec3(-1, -1, -1);   // 叶子节点用，对应叶子包含的三角形（最多3个，可拓展），其起始索引（在原始数据中），为局部（本Mesh）
    int left = -1;       // 内部节点索引
    int right = -1;      // 内部节点索引
};

class BLASBuilder {
public:
    struct Triangle {
        glm::vec3 v0, v1, v2;
        int index;  // 索引起始位置
    };

    struct BLAS {
        std::vector<BLASNode> nodes;
        std::vector<Triangle> triangles;
    };

    static BLAS buildBLAS(const Mesh* mesh);

private:
    static int buildRecursive(BLAS& blas, std::vector<Triangle>& tris, int begin, int end, std::vector<BLASNode>& nodes);

    inline static int maxAxis(const glm::vec3& v) {
        if (v.x > v.y && v.x > v.z) return 0;
        if (v.y > v.z) return 1;
        return 2;
    }
};

struct objectInfo{
    glm::mat4 transform;
    int materialID;
};

struct alignas(16) TLASInstance {
    glm::mat4 transform;
    AABB worldBounds;
    int rootNodeIndex; // 对应BLAS根节点位置
    int baseIndexOffset;
    int materialID;
    float padding;
};

class TLASBuilder {
public:
    struct alignas(16) TLASNode {
        AABB bounds;
        int left = -1;
        int right = -1;
        int instanceIndex = -1;  // 若为叶子节点
    };

    struct TLAS {
        std::vector<TLASInstance> instances;
        std::vector<TLASNode> nodes;
    };

    static TLAS buildTLAS(const std::vector<objectInfo>& objects, const std::vector<BLASBuilder::BLAS>& blasList);

private:
    static int buildRecursive(TLAS& tlas, int begin, int end, std::vector<TLASNode>& nodes);

    inline static int maxAxis(const glm::vec3& v) {
        if (v.x > v.y && v.x > v.z) return 0;
        if (v.y > v.z) return 1;
        return 2;
    }
};


// --------------------------------------------------------------------------------------
class BindableBuffer {
public:
    virtual void create() = 0;
    virtual void upload() = 0;
    virtual void writeDescriptor(descriptorSet& set, uint32_t binding) = 0;
    virtual ~BindableBuffer() = default;
};

template<typename T>
class TypedBufferManager : public BindableBuffer {
protected:
    std::unique_ptr<storageBuffer> buffer;
    std::vector<T> data;

public:
    virtual std::vector<T> fetch() const = 0;

    void create() override {
        data = fetch();
        if (!data.empty())
            buffer = std::make_unique<storageBuffer>(data.size() * sizeof(T));
    }

    void upload() override {
        if (!data.empty())
            buffer->TransferData(data.data(), data.size() * sizeof(T));
    }

    void writeDescriptor(descriptorSet& set, uint32_t binding) override {
        if (buffer) {
            VkDescriptorBufferInfo info{
                .buffer = buffer->getHandle(), .offset = 0, .range = VK_WHOLE_SIZE
            };
            set.Write(makeSpanFromOne(info), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding);
        }
    }
};

class VertexBufferManager : public TypedBufferManager<VertexData> {
private:
    std::vector<Mesh*> meshes;

public:
    void collect(const std::vector<std::unique_ptr<Mesh>>& input) {
        meshes.clear();
        meshes.reserve(input.size());
        for (const auto& m : input)
            meshes.push_back(m.get());  // 提取裸指针
    }
    void collect(const std::vector<std::shared_ptr<Mesh>>& input) {
        meshes.clear();
        meshes.reserve(input.size());
        for (const auto& m : input)
            meshes.push_back(m.get());  // 提取裸指针
    }

    std::vector<VertexData> fetch() const override {
        std::vector<VertexData> result;
        for (const auto& mesh : meshes) {
            const auto& verts = mesh->getVertices();
            result.insert(result.end(), verts.begin(), verts.end());
        }
        return result;
    }
};


class IndexBufferManager : public TypedBufferManager<uint32_t> {
private:
    std::vector<Mesh*> meshes;
    inline static uint32_t triCount;

public:
    static uint32_t getTriangleCount() { return triCount; } // 获取累计的三角形数量
    void collect(const std::vector<std::unique_ptr<Mesh>>& input) {
        meshes.clear();
        meshes.reserve(input.size());
        for (const auto& m : input)
            meshes.push_back(m.get());  // 提取裸指针
    }
    void collect(const std::vector<std::shared_ptr<Mesh>>& input) {
        meshes.clear();
        meshes.reserve(input.size());
        for (const auto& m : input)
            meshes.push_back(m.get());  // 提取裸指针
    }

    std::vector<uint32_t> fetch() const override {
        std::vector<uint32_t> result;
        uint32_t vertexOffset = 0;
        for (const auto& mesh : meshes) {
            const auto& meshIndices = mesh->getIndices();
            triCount += static_cast<uint32_t>(meshIndices.size() / 3); // 累计三角形数量
            for (uint32_t idx : meshIndices)
                result.push_back(idx + vertexOffset);
            vertexOffset += static_cast<uint32_t>(mesh->getVertices().size());
        }
        return result;
    }
};

class BLASBufferManager : public TypedBufferManager<BLASNode> {
private:
    std::vector<Mesh*> meshes;
    std::vector<BLASBuilder::BLAS> blasList;
    std::vector<objectInfo> obInfos;

public:
    void collect(const std::vector<std::unique_ptr<Mesh>>& input) {
        meshes.clear();
        blasList.clear();
        obInfos.clear(); 
        meshes.reserve(input.size());
        for (const auto& m : input) {
            objectInfo info;
            info.transform = m->get_ModelMatrix();
            info.materialID = m->get_materialID();

            obInfos.push_back(info);
            meshes.push_back(m.get());
            blasList.push_back(BLASBuilder::buildBLAS(m.get()));
        }
    }
    void collect(const std::vector<std::shared_ptr<Mesh>>& input) {
        meshes.clear();
        blasList.clear();
        obInfos.clear(); 
        meshes.reserve(input.size());
        for (const auto& m : input) {
            objectInfo info;
            info.transform = m->get_ModelMatrix();
            info.materialID = m->get_materialID();

            obInfos.push_back(info);
            meshes.push_back(m.get());
            blasList.push_back(BLASBuilder::buildBLAS(m.get()));
        }
    }

    std::vector<BLASNode> fetch() const override {
        std::vector<BLASNode> result;
        uint32_t offset = 0;
        for (const auto& blas : blasList)
        {
            for (const BLASNode& n : blas.nodes)
            {
                BLASNode copy = n;                 // 做一份拷贝再改
                if (copy.left  >= 0) copy.left  += offset;
                if (copy.right >= 0) copy.right += offset;
                result.push_back(copy);
            }
            offset += static_cast<uint32_t>(blas.nodes.size());
        }
        return result;
    }

    const std::vector<BLASBuilder::BLAS>& getBLASList() const { return blasList; }
    const std::vector<objectInfo>& getObjectInfo() const { return obInfos; }
};

class TLASBufferManager : public BindableBuffer {
private:
    std::vector<TLASInstance> instances;
    std::vector<TLASBuilder::TLASNode> nodes;

    std::unique_ptr<storageBuffer> instanceBuffer;
    std::unique_ptr<storageBuffer> nodeBuffer;

public:
    void build(const std::vector<objectInfo>& objectInfos, const std::vector<BLASBuilder::BLAS>& blasList) {
        auto tlas = TLASBuilder::buildTLAS(objectInfos, blasList);
        instances = std::move(tlas.instances);
        nodes     = std::move(tlas.nodes);
    }

    void create() override {
        if (!instances.empty())
            instanceBuffer = std::make_unique<storageBuffer>(instances.size() * sizeof(TLASInstance));
        if (!nodes.empty())
            nodeBuffer = std::make_unique<storageBuffer>(nodes.size() * sizeof(TLASBuilder::TLASNode));
    }

    void upload() override {
        if (instanceBuffer && !instances.empty())
            instanceBuffer->TransferData(instances.data(), instances.size() * sizeof(TLASInstance));
        if (nodeBuffer && !nodes.empty())
            nodeBuffer->TransferData(nodes.data(), nodes.size() * sizeof(TLASBuilder::TLASNode));
    }

    void writeDescriptor(descriptorSet& set, uint32_t baseBinding) override {
        if (instanceBuffer) {
            VkDescriptorBufferInfo info {
                .buffer = instanceBuffer->getHandle(),
                .offset = 0,
                .range = VK_WHOLE_SIZE
            };
            set.Write(makeSpanFromOne(info), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, baseBinding); 
        }

        if (nodeBuffer) {
            VkDescriptorBufferInfo info {
                .buffer = nodeBuffer->getHandle(),
                .offset = 0,
                .range = VK_WHOLE_SIZE
            };
            set.Write(makeSpanFromOne(info), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, baseBinding + 1); 
        }
    }
};

class MaterialBufferManager : public TypedBufferManager<MaterialData> {
public:
    std::vector<MaterialData> fetch() const override {
        return MaterialManager::get().getMaterialDataBuffer();
    }
};

class FaceLightBufferManager : public TypedBufferManager<FaceLightData> {
public:
    std::vector<FaceLightData> fetch() const override {
        return Lights::get().getFaceLightDataBuffer();
    }
};

class DirectionalLightBufferManager : public TypedBufferManager<DirectionalLightData> {
public:
    std::vector<DirectionalLightData> fetch() const override {
        return Lights::get().getDirectionalLightDataBuffer();
    }
};

// --------------------------------------------------------------------------------------
class Scene{
    struct alignas(16) SceneInfo{
        int skyboxIndex; 
        float skyboxIntensity;
    };
private:
    std::vector<std::shared_ptr<Mesh>> s_meshes;
    std::unique_ptr<Camera> s_camera;

    std::unique_ptr<descriptorSetLayout> s_descriptorSetLayout;
    std::unique_ptr<descriptorSet> s_descriptorSet;

    std::unique_ptr<VertexBufferManager>         s_vertexBufferMgr;
    std::unique_ptr<IndexBufferManager>          s_indexBufferMgr;
    std::unique_ptr<BLASBufferManager>          s_blasBufferMgr;
    std::unique_ptr<TLASBufferManager>          s_tlasBufferMgr;
    std::unique_ptr<MaterialBufferManager>       s_materialBufferMgr;
    std::unique_ptr<FaceLightBufferManager>      s_faceLightBufferMgr;
    std::unique_ptr<DirectionalLightBufferManager> s_directionalLightBufferMgr;
    
    SceneInfo s_sceneInfo; 
    std::unique_ptr<uniformBuffer> s_sceneInfoBuffer;
public:
    void initScene();
    void initDescriptor(descriptorPool* pool);
    void writeDescriptor();
    void update(float deltaTime);

    inline VkDescriptorSetLayout getDescriptorSetLayout() {return s_descriptorSetLayout->getHandle();}
    inline VkDescriptorSet getDescriptorSet(){return s_descriptorSet->getHandle();}
    inline Camera* getCamera() {return s_camera.get();}
    inline uint32_t getTriCount() const {
        return IndexBufferManager::getTriangleCount();
    }

private:
    void initBufferManager();
    void uploadSceneToGPU();

public:
    static glm::vec3 hexToVec3(const std::string& hexStr);
};