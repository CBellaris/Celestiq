#pragma once
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp> // 实验功能，使用了glm::toMat4()

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "vulkan/vulkan.h"
#include "VKBase.h"
#include "Material.h"

class Shader;

struct alignas(16) VertexData
{
    alignas(16) glm::vec3 Position;

    alignas(16) glm::vec3 Normal;
    alignas(16) glm::vec3 Tangent;   // 切线
    alignas(8) glm::vec2 TexCoords;
    float tangentW;      // 切线方向标志位（+1 或 -1）

    VertexData(float px = 0.0f, float py = 0.0f, float pz = 0.0f,
               float nx = 0.0f, float ny = 0.0f, float nz = 0.0f,
               float tx = 0.0f, float ty = 0.0f)
               : Position(px, py, pz), Normal(nx, ny, nz), TexCoords(tx, ty), 
               Tangent(glm::vec3(0.0f, 0.0f, 0.0f)), tangentW(1.0f) {}
};

class Mesh
{
private:
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;

    glm::mat4 model;
    glm::vec3 position;
    glm::vec3 eulerAngles; // 存储用户设置的欧拉角（旋转角度）
    glm::vec3 scale;

    // 可视性
    bool visibility;

    uint32_t materialID;

    Celestiq::Vulkan::vertexBuffer vertexBuffer;    // 仅在图像管线使用

public:
    Mesh();
    ~Mesh() = default;

    void set_mesh(std::vector<VertexData> vertices, std::vector<unsigned int> indices, bool if_Cal_Tangents = false);
    inline void set_materialID(uint32_t materialID) { this->materialID = materialID; }

    void create_vertex_buffer(); // 仅在图像管线使用

    // 静态方法：获取顶点绑定描述
    static const VkVertexInputBindingDescription& getBindingDescription();
    // 静态方法：获取顶点属性描述
    static const std::array<VkVertexInputAttributeDescription, 5>& getAttributeDescriptions();
    static void bind_pipeline(Celestiq::Vulkan::graphicsPipelineCreateInfoPack& pipelineCiPack);

    void set_mesh_screen();
    void set_mesh_plane();
    void set_mesh_cube();
    void set_mesh_box();
    void set_mesh_sphere(int sectorCount, int stackCount, float radius = 1.0f);

    // 设置模型的位移
    void set_position(const glm::vec3& pos) ;
    // 设置模型的旋转（使用角度）
    void set_rotation(const glm::vec3& angles);
    // 设置模型的缩放
    void set_scale(const glm::vec3& scl);

    // 获取接口
    inline const glm::vec3& get_position(){return position;}
    inline size_t get_bufferSizeOfVertex() { return sizeof(VertexData) * vertices.size(); }
    inline size_t get_bufferSizeOfIndex() { return sizeof(uint32_t) * indices.size(); }
    inline int get_sizeOfIndex() { return indices.size(); }
    inline const uint32_t get_materialID() { return materialID; }
    const std::vector<VertexData>& getVertices() const { return vertices; }
    const std::vector<uint32_t>& getIndices() const { return indices; }

    // 获取当前的模型矩阵
    inline const glm::mat4& get_ModelMatrix() const {
        return model;
    }

    // 设置可视性
    void set_visibility(const bool visiable);
    inline bool get_visibility(){return visibility;}

    void draw(VkCommandBuffer commandBuffer);   // 仅在图像管线使用

private:
    // 更新模型矩阵
    void updateModelMatrix();

    // 切线和副切线计算函数
    void calculateTangents(
        const std::vector<VertexData>& vertices,
        const std::vector<unsigned int>& indices,
        std::vector<glm::vec3>& tangents);
    void calculateTangentsAndBitangents(const std::vector<VertexData>& vertices,
                                          const std::vector<unsigned int>& indices,
                                          std::vector<glm::vec3>& tangents,
                                          std::vector<glm::vec3>& bitangents);
};

class Model
{
    friend class Scene;
private:
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::string directory;

    void loadModel(std::string path);
    void processNode(aiNode *node, const aiScene *scene);
    std::shared_ptr<Mesh> processMesh(aiMesh *mesh, const aiScene *scene);

    void process_material_textures(
        aiMaterial* material,
        aiTextureType assimpType,
        const aiScene* scene,
        MaterialData& mat,
        const std::string& directory
    );
public:
    Model(std::string path){loadModel(path);}
    inline void set_position(const glm::vec3& pos) {for(auto &mesh : meshes) mesh->set_position(pos);}
    inline void set_scale(const glm::vec3& scl) {for(auto &mesh : meshes) mesh->set_scale(scl);}
    inline void set_rotation(const glm::vec3& rot) {for(auto &mesh : meshes) mesh->set_rotation(rot);}
    inline void set_materialID(uint32_t materialID) {for(auto &mesh : meshes) mesh->set_materialID(materialID);}
};