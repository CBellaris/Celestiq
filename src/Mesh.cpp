#include "Mesh.h"

// 静态方法：获取顶点绑定描述
VkVertexInputBindingDescription Mesh::getBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0; // 绑定编号
    bindingDescription.stride = sizeof(Vertexdata); // 每个顶点步进
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // 每个顶点一次
    return bindingDescription;
}

// 静态方法：获取顶点属性描述
std::array<VkVertexInputAttributeDescription, 5> Mesh::getAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
    attributeDescriptions[0].offset = offsetof(Vertexdata, Position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
    attributeDescriptions[1].offset = offsetof(Vertexdata, Normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // vec2
    attributeDescriptions[2].offset = offsetof(Vertexdata, TexCoords);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
    attributeDescriptions[3].offset = offsetof(Vertexdata, Tangent);

    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32_SFLOAT; // float
    attributeDescriptions[4].offset = offsetof(Vertexdata, tangentW);

    return attributeDescriptions;
}

void Mesh::bind_pipeline(Celestiq::Vulkan::graphicsPipelineCreateInfoPack &pipelineCiPack)
{
    auto bindingDescription = Mesh::getBindingDescription();
    auto attributeDescriptions = Mesh::getAttributeDescriptions();

    pipelineCiPack.vertexInputStateCi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineCiPack.vertexInputStateCi.vertexBindingDescriptionCount = 1;
    pipelineCiPack.vertexInputStateCi.pVertexBindingDescriptions = &bindingDescription;
    pipelineCiPack.vertexInputStateCi.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    pipelineCiPack.vertexInputStateCi.pVertexAttributeDescriptions = attributeDescriptions.data();
}

Mesh::Mesh(): model(glm::mat4(1.0f)), position(0.0f), eulerAngles(0.0f), scale(1.0f), visibility(true)
{ 
    // pass
}


void Mesh::set_mesh(std::vector<Vertexdata> vertices, std::vector<unsigned int> indices, bool if_Cal_Tangents)
{
    this->vertices = vertices;
    this->indices = indices;

    if (if_Cal_Tangents) {
        // 计算切线和副切线
        std::vector<glm::vec3> tangents;
        calculateTangents(vertices, indices, tangents);

        // 将计算的切线和副切线添加到顶点数据
        for (size_t i = 0; i < vertices.size(); ++i) {
            vertices[i].Tangent = tangents[i];
            vertices[i].tangentW = 1.0f; // 默认方向为 +1
        }
    }

    create_vertex_buffer();
}

void Mesh::create_vertex_buffer()
{
    // 1. 定义创建 VkBuffer 的参数
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.size = sizeof(Vertexdata) * vertices.size();
    bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // 2. 创建 buffer + 分配 memory + 绑定 memory
    vertexBuffer.Create(sizeof(Vertexdata) * vertices.size());

    // 3. 上传顶点数据
    vertexBuffer.TransferData(vertices.data(), sizeof(Vertexdata) * vertices.size());
}


void Mesh::set_mesh_screen()
{
    std::vector<Vertexdata> verticesCube = {
    Vertexdata(-1.0f, -1.0f,  0.0f,   0.0f,  0.0f, 1.0f, 0.0f, 0.0f),
    Vertexdata( 1.0f, -1.0f,  0.0f,   0.0f,  0.0f, 1.0f, 1.0f, 0.0f),
    Vertexdata(-1.0f,  1.0f,  0.0f,   0.0f,  0.0f, 1.0f, 0.0f, 1.0f),
    Vertexdata(-1.0f,  1.0f,  0.0f,   0.0f,  0.0f, 1.0f, 0.0f, 1.0f),
    Vertexdata( 1.0f, -1.0f,  0.0f,   0.0f,  0.0f, 1.0f, 1.0f, 0.0f),
    Vertexdata( 1.0f,  1.0f,  0.0f,   0.0f,  0.0f, 1.0f, 1.0f, 1.0f)};
    
    std::vector<unsigned int> indicesDummy;
    for (unsigned int i=0; i<verticesCube.size(); i++)
        indicesDummy.push_back(i);
    set_mesh(verticesCube, indicesDummy, true);
}

void Mesh::set_mesh_plane()
{
    std::vector<Vertexdata> verticesCube = {
        Vertexdata( 1.0f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f, 1.0f, 0.0f),
        Vertexdata( 1.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f, 1.0f, 1.0f),
        Vertexdata(-1.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f, 0.0f, 1.0f),
        Vertexdata(-1.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f, 0.0f, 1.0f),
        Vertexdata(-1.0f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f, 0.0f, 0.0f),
        Vertexdata( 1.0f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f, 1.0f, 0.0f)};

    std::vector<unsigned int> indicesDummy;
    for (unsigned int i=0; i<verticesCube.size(); i++)
        indicesDummy.push_back(i);
    set_mesh(verticesCube, indicesDummy, true);
}

// 快速创建一个立方体
void Mesh::set_mesh_cube()
{
    std::vector<Vertexdata> verticesCube = {
    Vertexdata( 1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f, 1.0f, 1.0f),
    Vertexdata( 1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f, 1.0f, 0.0f),
    Vertexdata(-1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f, 0.0f, 0.0f),
    Vertexdata(-1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f, 0.0f, 0.0f),
    Vertexdata(-1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f, 0.0f, 1.0f), 
    Vertexdata( 1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f, 1.0f, 1.0f),
  
    Vertexdata(-1.0f, -1.0f,  1.0f,   0.0f,  0.0f, 1.0f, 0.0f, 0.0f),
    Vertexdata( 1.0f, -1.0f,  1.0f,   0.0f,  0.0f, 1.0f, 0.0f, 1.0f),
    Vertexdata( 1.0f,  1.0f,  1.0f,   0.0f,  0.0f, 1.0f, 1.0f, 1.0f),
    Vertexdata( 1.0f,  1.0f,  1.0f,   0.0f,  0.0f, 1.0f, 1.0f, 1.0f),
    Vertexdata(-1.0f,  1.0f,  1.0f,   0.0f,  0.0f, 1.0f, 1.0f, 0.0f),
    Vertexdata(-1.0f, -1.0f,  1.0f,   0.0f,  0.0f, 1.0f, 0.0f, 0.0f), 
  
    Vertexdata(-1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f, 0.0f, 1.0f),
    Vertexdata(-1.0f,  1.0f, -1.0f,  -1.0f,  0.0f,  0.0f, 1.0f, 1.0f),
    Vertexdata(-1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f, 1.0f, 0.0f),
    Vertexdata(-1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f, 1.0f, 0.0f),
    Vertexdata(-1.0f, -1.0f,  1.0f,  -1.0f,  0.0f,  0.0f, 0.0f, 0.0f),
    Vertexdata(-1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f, 0.0f, 1.0f), 
  
    Vertexdata( 1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f, 0.0f, 1.0f),
    Vertexdata( 1.0f,  1.0f, -1.0f,   1.0f,  0.0f,  0.0f, 1.0f, 1.0f),
    Vertexdata( 1.0f,  1.0f,  1.0f,   1.0f,  0.0f,  0.0f, 1.0f, 0.0f),
    Vertexdata( 1.0f,  1.0f,  1.0f,   1.0f,  0.0f,  0.0f, 1.0f, 0.0f),
    Vertexdata( 1.0f, -1.0f,  1.0f,   1.0f,  0.0f,  0.0f, 0.0f, 0.0f),
    Vertexdata( 1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f, 0.0f, 1.0f),

    Vertexdata(-1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f, 0.0f, 1.0f),
    Vertexdata( 1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f, 1.0f, 1.0f),
    Vertexdata( 1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f, 1.0f, 0.0f),
    Vertexdata( 1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f, 1.0f, 0.0f),
    Vertexdata(-1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f, 0.0f, 0.0f),
    Vertexdata(-1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f, 0.0f, 1.0f), 
  
    Vertexdata( 1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f, 0.0f, 1.0f),
    Vertexdata( 1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f, 1.0f, 1.0f),
    Vertexdata(-1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f, 1.0f, 0.0f),
    Vertexdata(-1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f, 1.0f, 0.0f),
    Vertexdata(-1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f, 0.0f, 0.0f),
    Vertexdata( 1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f, 0.0f, 1.0f)
    };

    std::vector<unsigned int> indicesDummy;
    for (unsigned int i=0; i<verticesCube.size(); i++)
        indicesDummy.push_back(i);
    set_mesh(verticesCube, indicesDummy, true);
}

void Mesh::set_mesh_sphere(int sectorCount, int stackCount, float radius)
{
    std::vector<Vertexdata> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;
    for (int i = 0; i <= stackCount; ++i) {
        float stackAngle = PI / 2 - i * (PI / stackCount); // 从 π/2 到 -π/2
        float xy = radius * cosf(stackAngle);              // r * cos(phi)
        float z = radius * sinf(stackAngle);               // r * sin(phi)

        for (int j = 0; j <= sectorCount; ++j) {
            float sectorAngle = j * (2 * PI / sectorCount); // 从 0 到 2π

            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            float nx = x / radius;
            float ny = y / radius;
            float nz = z / radius;
            float u = (float)j / sectorCount;
            float v = (float)i / stackCount;

            vertices.emplace_back(Vertexdata(x, y, z, nx, ny, nz, u, v));
        }
    }

    // 创建索引（GL_TRIANGLES）
    for (int i = 0; i < stackCount; ++i) {
        int k1 = i * (sectorCount + 1);
        int k2 = k1 + sectorCount + 1;

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    set_mesh(vertices, indices, true);
}


void Mesh::set_position(const glm::vec3 &pos)
{
    position = pos;
    updateModelMatrix();
}
void Mesh::set_rotation(const glm::vec3& angles)
{
    eulerAngles = angles;
    updateModelMatrix();
}
void Mesh::set_scale(const glm::vec3& scl) 
{
    scale = scl;
    updateModelMatrix();
}

void Mesh::set_visibility(const bool visiable)
{
    visibility = visiable;
}

void Mesh::draw(VkCommandBuffer commandBuffer)
{
    if(visibility){
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer.Address(), &offset);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }
}


void Mesh::updateModelMatrix()
{
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
    glm::mat4 rotationMatrix = glm::toMat4(glm::quat(glm::radians(eulerAngles)));  // 使用四元数生成旋转矩阵
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);

    model = translationMatrix * rotationMatrix * scaleMatrix;
}

void Mesh::calculateTangents(const std::vector<Vertexdata> &vertices, const std::vector<unsigned int> &indices, std::vector<glm::vec3> &tangents)
{
    tangents.resize(vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i < indices.size(); i += 3) {
        const Vertexdata& v0 = vertices[indices[i]];
        const Vertexdata& v1 = vertices[indices[i + 1]];
        const Vertexdata& v2 = vertices[indices[i + 2]];

        glm::vec3 edge1 = glm::vec3(v1.Position.x - v0.Position.x, v1.Position.y - v0.Position.y, v1.Position.z - v0.Position.z);
        glm::vec3 edge2 = glm::vec3(v2.Position.x - v0.Position.x, v2.Position.y - v0.Position.y, v2.Position.z - v0.Position.z);

        glm::vec2 deltaUV1 = glm::vec2(v1.TexCoords.x - v0.TexCoords.x, v1.TexCoords.y - v0.TexCoords.y);
        glm::vec2 deltaUV2 = glm::vec2(v2.TexCoords.x - v0.TexCoords.x, v2.TexCoords.y - v0.TexCoords.y);

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        glm::vec3 tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);

        tangents[indices[i]] += tangent;
        tangents[indices[i + 1]] += tangent;
        tangents[indices[i + 2]] += tangent;
    }

    for (size_t i = 0; i < tangents.size(); ++i) {
        tangents[i] = glm::normalize(tangents[i]);
    }
}