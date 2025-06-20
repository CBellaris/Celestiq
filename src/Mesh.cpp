#include "Mesh.h"

// 静态方法：获取顶点绑定描述
const VkVertexInputBindingDescription& Mesh::getBindingDescription()
{
    static const VkVertexInputBindingDescription bindingDescription = {0, sizeof(VertexData), VK_VERTEX_INPUT_RATE_VERTEX};
    return bindingDescription;
}

// 静态方法：获取顶点属性描述
const std::array<VkVertexInputAttributeDescription, 5>& Mesh::getAttributeDescriptions()
{
    static const std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = [] {
        std::array<VkVertexInputAttributeDescription, 5> arr{};

        arr[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexData, Position)};
        arr[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexData, Normal)};
        arr[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(VertexData, TexCoords)};
        arr[3] = {3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexData, Tangent)};
        arr[4] = {4, 0, VK_FORMAT_R32_SFLOAT,        offsetof(VertexData, tangentW)};

        return arr;
    }();

    return attributeDescriptions;
}

void Mesh::bind_pipeline(Celestiq::Vulkan::graphicsPipelineCreateInfoPack &pipelineCiPack)
{
    const auto& bindingDescription = Mesh::getBindingDescription();
    const auto& attributeDescriptions = Mesh::getAttributeDescriptions();

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


void Mesh::set_mesh(std::vector<VertexData> vertices, std::vector<unsigned int> indices, bool if_Cal_Tangents)
{
    this->vertices = vertices;
    this->indices = indices;

    if (if_Cal_Tangents) {
        std::vector<glm::vec3> tangents;
        std::vector<glm::vec3> bitangents;

        calculateTangentsAndBitangents(this->vertices, this->indices, tangents, bitangents);

        // 为每个顶点计算正交化的TBN和手性
        for (size_t i = 0; i < this->vertices.size(); ++i) {
            const glm::vec3& n = this->vertices[i].Normal;
            const glm::vec3& t = tangents[i];
            const glm::vec3& b = bitangents[i];

            // Gram-Schmidt 正交化
            glm::vec3 tangent = glm::normalize(t - n * glm::dot(n, t));

            // 计算手性. dot(cross(N, T'), B)
            // 如果结果 > 0, 说明 B 和 cross(N, T') 方向一致, 是右手系
            // 如果结果 < 0, 说明 B 和 cross(N, T') 方向相反, 是左手系
            float handedness = (glm::dot(glm::cross(n, tangent), b) > 0.0f) ? 1.0f : -1.0f;

            this->vertices[i].Tangent = tangent;
            this->vertices[i].tangentW = handedness;
        }
    }
}

void Mesh::create_vertex_buffer()
{
    vertexBuffer.Create(sizeof(VertexData) * vertices.size());
    vertexBuffer.TransferData(vertices.data(), sizeof(VertexData) * vertices.size());
}


void Mesh::set_mesh_screen()
{
    std::vector<VertexData> verticesCube = {
    VertexData(-1.0f, -1.0f,  0.0f,   0.0f,  0.0f, 1.0f, 0.0f, 0.0f),
    VertexData( 1.0f, -1.0f,  0.0f,   0.0f,  0.0f, 1.0f, 1.0f, 0.0f),
    VertexData(-1.0f,  1.0f,  0.0f,   0.0f,  0.0f, 1.0f, 0.0f, 1.0f),
    VertexData(-1.0f,  1.0f,  0.0f,   0.0f,  0.0f, 1.0f, 0.0f, 1.0f),
    VertexData( 1.0f, -1.0f,  0.0f,   0.0f,  0.0f, 1.0f, 1.0f, 0.0f),
    VertexData( 1.0f,  1.0f,  0.0f,   0.0f,  0.0f, 1.0f, 1.0f, 1.0f)};
    
    std::vector<unsigned int> indicesDummy;
    for (unsigned int i=0; i<verticesCube.size(); i++)
        indicesDummy.push_back(i);
    set_mesh(verticesCube, indicesDummy, true);
}

void Mesh::set_mesh_plane()
{
    std::vector<VertexData> verticesCube = {
        VertexData( 1.0f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f, 1.0f, 0.0f),
        VertexData( 1.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f, 1.0f, 1.0f),
        VertexData(-1.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f, 0.0f, 1.0f),
        VertexData(-1.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f, 0.0f, 1.0f),
        VertexData(-1.0f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f, 0.0f, 0.0f),
        VertexData( 1.0f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f, 1.0f, 0.0f)};

    std::vector<unsigned int> indicesDummy;
    for (unsigned int i=0; i<verticesCube.size(); i++)
        indicesDummy.push_back(i);
    set_mesh(verticesCube, indicesDummy, true);
}

// 快速创建一个立方体
void Mesh::set_mesh_cube()
{
    std::vector<VertexData> verticesCube = {
    VertexData( 1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f, 1.0f, 1.0f),
    VertexData( 1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f, 1.0f, 0.0f),
    VertexData(-1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f, 0.0f, 0.0f),
    VertexData(-1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f, 0.0f, 0.0f),
    VertexData(-1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f, 0.0f, 1.0f), 
    VertexData( 1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f, 1.0f, 1.0f),
  
    VertexData(-1.0f, -1.0f,  1.0f,   0.0f,  0.0f, 1.0f, 0.0f, 0.0f),
    VertexData( 1.0f, -1.0f,  1.0f,   0.0f,  0.0f, 1.0f, 0.0f, 1.0f),
    VertexData( 1.0f,  1.0f,  1.0f,   0.0f,  0.0f, 1.0f, 1.0f, 1.0f),
    VertexData( 1.0f,  1.0f,  1.0f,   0.0f,  0.0f, 1.0f, 1.0f, 1.0f),
    VertexData(-1.0f,  1.0f,  1.0f,   0.0f,  0.0f, 1.0f, 1.0f, 0.0f),
    VertexData(-1.0f, -1.0f,  1.0f,   0.0f,  0.0f, 1.0f, 0.0f, 0.0f), 
  
    VertexData(-1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f, 0.0f, 1.0f),
    VertexData(-1.0f,  1.0f, -1.0f,  -1.0f,  0.0f,  0.0f, 1.0f, 1.0f),
    VertexData(-1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f, 1.0f, 0.0f),
    VertexData(-1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f, 1.0f, 0.0f),
    VertexData(-1.0f, -1.0f,  1.0f,  -1.0f,  0.0f,  0.0f, 0.0f, 0.0f),
    VertexData(-1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f, 0.0f, 1.0f), 
  
    VertexData( 1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f, 0.0f, 1.0f),
    VertexData( 1.0f,  1.0f, -1.0f,   1.0f,  0.0f,  0.0f, 1.0f, 1.0f),
    VertexData( 1.0f,  1.0f,  1.0f,   1.0f,  0.0f,  0.0f, 1.0f, 0.0f),
    VertexData( 1.0f,  1.0f,  1.0f,   1.0f,  0.0f,  0.0f, 1.0f, 0.0f),
    VertexData( 1.0f, -1.0f,  1.0f,   1.0f,  0.0f,  0.0f, 0.0f, 0.0f),
    VertexData( 1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f, 0.0f, 1.0f),

    VertexData(-1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f, 0.0f, 1.0f),
    VertexData( 1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f, 1.0f, 1.0f),
    VertexData( 1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f, 1.0f, 0.0f),
    VertexData( 1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f, 1.0f, 0.0f),
    VertexData(-1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f, 0.0f, 0.0f),
    VertexData(-1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f, 0.0f, 1.0f), 
  
    VertexData( 1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f, 0.0f, 1.0f),
    VertexData( 1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f, 1.0f, 1.0f),
    VertexData(-1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f, 1.0f, 0.0f),
    VertexData(-1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f, 1.0f, 0.0f),
    VertexData(-1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f, 0.0f, 0.0f),
    VertexData( 1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f, 0.0f, 1.0f)
    };

    std::vector<unsigned int> indicesDummy;
    for (unsigned int i=0; i<verticesCube.size(); i++)
        indicesDummy.push_back(i);
    set_mesh(verticesCube, indicesDummy, true);
}

void Mesh::set_mesh_box()
{
    std::vector<VertexData> verticesBox = {
    VertexData( 1.0f,  1.0f, -1.0f,   0.0f,  0.0f, 1.0f, 1.0f, 1.0f),
    VertexData( 1.0f, -1.0f, -1.0f,   0.0f,  0.0f, 1.0f, 1.0f, 0.0f),
    VertexData(-1.0f, -1.0f, -1.0f,   0.0f,  0.0f, 1.0f, 0.0f, 0.0f),
    VertexData(-1.0f, -1.0f, -1.0f,   0.0f,  0.0f, 1.0f, 0.0f, 0.0f),
    VertexData(-1.0f,  1.0f, -1.0f,   0.0f,  0.0f, 1.0f, 0.0f, 1.0f), 
    VertexData( 1.0f,  1.0f, -1.0f,   0.0f,  0.0f, 1.0f, 1.0f, 1.0f),
  
    // VertexData(-1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f),
    // VertexData(-1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f),
    // VertexData(-1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f),
    // VertexData(-1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f),
    // VertexData(-1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f),
    // VertexData(-1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f), 
  
    // VertexData( 1.0f, -1.0f, -1.0f,   -1.0f,  0.0f,  0.0f, 0.0f, 1.0f),
    // VertexData( 1.0f,  1.0f, -1.0f,   -1.0f,  0.0f,  0.0f, 1.0f, 1.0f),
    // VertexData( 1.0f,  1.0f,  1.0f,   -1.0f,  0.0f,  0.0f, 1.0f, 0.0f),
    // VertexData( 1.0f,  1.0f,  1.0f,   -1.0f,  0.0f,  0.0f, 1.0f, 0.0f),
    // VertexData( 1.0f, -1.0f,  1.0f,   -1.0f,  0.0f,  0.0f, 0.0f, 0.0f),
    // VertexData( 1.0f, -1.0f, -1.0f,   -1.0f,  0.0f,  0.0f, 0.0f, 1.0f),

    VertexData(-1.0f, -1.0f, -1.0f,   0.0f, 1.0f,  0.0f, 0.0f, 1.0f),
    VertexData( 1.0f, -1.0f, -1.0f,   0.0f, 1.0f,  0.0f, 1.0f, 1.0f),
    VertexData( 1.0f, -1.0f,  1.0f,   0.0f, 1.0f,  0.0f, 1.0f, 0.0f),
    VertexData( 1.0f, -1.0f,  1.0f,   0.0f, 1.0f,  0.0f, 1.0f, 0.0f),
    VertexData(-1.0f, -1.0f,  1.0f,   0.0f, 1.0f,  0.0f, 0.0f, 0.0f),
    VertexData(-1.0f, -1.0f, -1.0f,   0.0f, 1.0f,  0.0f, 0.0f, 1.0f), 
  
    VertexData( 1.0f,  1.0f,  1.0f,   0.0f,  -1.0f,  0.0f, 0.0f, 1.0f),
    VertexData( 1.0f,  1.0f, -1.0f,   0.0f,  -1.0f,  0.0f, 1.0f, 1.0f),
    VertexData(-1.0f,  1.0f, -1.0f,   0.0f,  -1.0f,  0.0f, 1.0f, 0.0f),
    VertexData(-1.0f,  1.0f, -1.0f,   0.0f,  -1.0f,  0.0f, 1.0f, 0.0f),
    VertexData(-1.0f,  1.0f,  1.0f,   0.0f,  -1.0f,  0.0f, 0.0f, 0.0f),
    VertexData( 1.0f,  1.0f,  1.0f,   0.0f,  -1.0f,  0.0f, 0.0f, 1.0f)
    };

    std::vector<unsigned int> indicesDummy;
    for (unsigned int i=0; i<verticesBox.size(); i++)
        indicesDummy.push_back(i);
    set_mesh(verticesBox, indicesDummy, true);
}

void Mesh::set_mesh_sphere(int sectorCount, int stackCount, float radius)
{
    std::vector<VertexData> vertices;
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

            vertices.emplace_back(VertexData(x, y, z, nx, ny, nz, u, v));
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
        vkCmdDraw(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0);
    }
}


void Mesh::updateModelMatrix()
{
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
    glm::mat4 rotationMatrix = glm::toMat4(glm::quat(glm::radians(eulerAngles)));  // 使用四元数生成旋转矩阵
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);

    model = translationMatrix * rotationMatrix * scaleMatrix;
}

void Mesh::calculateTangents(const std::vector<VertexData> &vertices, const std::vector<unsigned int> &indices, std::vector<glm::vec3> &tangents)
{
    tangents.resize(vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i < indices.size(); i += 3) {
        const VertexData& v0 = vertices[indices[i]];
        const VertexData& v1 = vertices[indices[i + 1]];
        const VertexData& v2 = vertices[indices[i + 2]];

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

void Mesh::calculateTangentsAndBitangents(const std::vector<VertexData>& vertices,
                                          const std::vector<unsigned int>& indices,
                                          std::vector<glm::vec3>& tangents,
                                          std::vector<glm::vec3>& bitangents)
{
    tangents.resize(vertices.size(), glm::vec3(0.0f));
    bitangents.resize(vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i < indices.size(); i += 3) {
        const VertexData& v0 = vertices[indices[i]];
        const VertexData& v1 = vertices[indices[i + 1]];
        const VertexData& v2 = vertices[indices[i + 2]];

        glm::vec3 edge1 = glm::vec3(v1.Position) - glm::vec3(v0.Position);
        glm::vec3 edge2 = glm::vec3(v2.Position) - glm::vec3(v0.Position);

        glm::vec2 deltaUV1 = glm::vec2(v1.TexCoords) - glm::vec2(v0.TexCoords);
        glm::vec2 deltaUV2 = glm::vec2(v2.TexCoords) - glm::vec2(v0.TexCoords);

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        
        // 如果 f 是无穷大或NaN，说明UV坐标重合或共线，跳过这个三角面
        if (std::isinf(f) || std::isnan(f)) {
            continue;
        }

        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        glm::vec3 bitangent;
        bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        // 累加到每个顶点
        tangents[indices[i]] += tangent;
        tangents[indices[i + 1]] += tangent;
        tangents[indices[i + 2]] += tangent;

        bitangents[indices[i]] += bitangent;
        bitangents[indices[i + 1]] += bitangent;
        bitangents[indices[i + 2]] += bitangent;
    }

    // 这里不进行normalize，因为我们需要在计算手性之后再做正交化
}

void Model::loadModel(std::string path)
{
    Assimp::Importer import;
    const aiScene *scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs| aiProcess_CalcTangentSpace);    

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
    // 处理节点所有的网格（如果有的话）
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
        meshes.push_back(processMesh(mesh, scene));         
    }
    // 接下来对它的子节点重复这一过程
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

std::shared_ptr<Mesh> Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    std::vector<VertexData> vertices;
    std::vector<unsigned int> indices;

    // --- 调试：检查UV通道数量 ---
    unsigned int numUVChannels = mesh->GetNumUVChannels();
    if (numUVChannels > 1) {
        std::cout << "Mesh '" << mesh->mName.C_Str() 
                  << "' has " << numUVChannels << " UV channels." << std::endl;
    }

    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        VertexData vertex;
        // 处理顶点位置、法线、纹理坐标和切线及副切线
        glm::vec3 vector; 
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z; 
        vertex.Position = vector;

        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.Normal = vector;

        vector.x = mesh->mTangents[i].x;
        vector.y = mesh->mTangents[i].y;
        vector.z = mesh->mTangents[i].z;
        vertex.Tangent = vector;

        if(mesh->mTextureCoords[0]) // 网格是否有纹理坐标？
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x; 
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        }
        else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }
    // 处理索引
    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }


    // 处理材质
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex]; 

    MaterialData mat;
    if (mesh->mMaterialIndex >= 0) {
        process_material_textures(material, aiTextureType_DIFFUSE, scene, mat, this->directory); 
        // process_material_textures(material, aiTextureType_METALNESS,  scene, mat, this->directory);  
        // process_material_textures(material, aiTextureType_DIFFUSE_ROUGHNESS, scene, mat, this->directory);  
        // process_material_textures(material, aiTextureType_NORMALS, scene, mat, this->directory);
    }
    uint32_t matID = MaterialManager::get().createMaterial(mat);

    std::shared_ptr<Mesh> mesh_ = std::make_shared<Mesh>();
    mesh_->set_mesh(vertices, indices);
    mesh_->set_materialID(matID);
    
    return std::move(mesh_);
}

// 目前一共有三种贴图加载方式，分别是：
// 1. 嵌入贴图-压缩图片 (e.g., PNG/JPG)（纹理数据嵌入到模型文件中）
// 2. 嵌入贴图-非压缩图像数据（像素数组）（纹理数据嵌入到模型文件中）
// 3. 外部贴图（纹理数据存储在外部文件中）
void Model::process_material_textures(
    aiMaterial* material,
    aiTextureType assimpType,
    const aiScene* scene,
    MaterialData& mat,
    const std::string& directory
) {
    unsigned int texCount = material->GetTextureCount(assimpType);
    if(texCount == 0) return;
    const unsigned int Process_TexCount = 1;
    if(texCount > Process_TexCount)
        std::cout<<"Multiple textures of one type, only process the first"<<std::endl;

    VkFormat imageFormat;
    if(assimpType == aiTextureType_DIFFUSE)
        imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
    else if(assimpType == aiTextureType_NORMALS)
        imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    else
        imageFormat = VK_FORMAT_R8_UNORM;

    for (unsigned int i = 0; i < Process_TexCount; ++i) {

        // --- 调试： 检查UV变换 ---
        aiUVTransform transform;
        if (material->Get(AI_MATKEY_UVTRANSFORM(assimpType, i), transform) == AI_SUCCESS) {
            std::cout << "=========================================================\n";
            std::cout << "Found UV Transform for material!\n";
            std::cout << "  - Texture Type: " << assimpType << ", Index: " << i << "\n";
            std::cout << "  - Translation: (" << transform.mTranslation.x << ", " << transform.mTranslation.y << ")\n";
            std::cout << "  - Scaling: (" << transform.mScaling.x << ", " << transform.mScaling.y << ")\n";
            std::cout << "  - Rotation (degrees): " << transform.mRotation * 180.0f / 3.14159265f << "\n";
            std::cout << "=========================================================\n";
        }

        aiString str;
        material->GetTexture(assimpType, i, &str);
        std::string texPath = str.C_Str();

        const aiTexture* embeddedTex = nullptr;

        // Assimp 样式内嵌贴图路径 "*0"
        if (!texPath.empty() && texPath[0] == '*') {
            int index = std::stoi(texPath.substr(1));
            if (index >= 0 && index < static_cast<int>(scene->mNumTextures)) {
                embeddedTex = scene->mTextures[index];
            }
        }

        // FBX 虚拟路径，如 "scene.fbm/Tex_0001.png"
        if (!embeddedTex) {
            for (unsigned int t = 0; t < scene->mNumTextures; ++t) {
                if (scene->mTextures[t]->mFilename.C_Str() == texPath) {
                    embeddedTex = scene->mTextures[t];
                    break;
                }
            }
        }

        uint32_t textureID = 0;
        if (embeddedTex) {
            // 嵌入贴图处理
            if (embeddedTex->mHeight == 0) {
                // 压缩图像（PNG/JPG）
                const uint8_t* data = reinterpret_cast<uint8_t*>(embeddedTex->pcData);
                size_t size = embeddedTex->mWidth;
                textureID = TextureManager::get().loadTexture(texPath, data, size, imageFormat);
            } else {
                // 原始像素图（一般是 BGRA 格式）
                const uint8_t* raw = reinterpret_cast<uint8_t*>(embeddedTex->pcData);
                int width = embeddedTex->mWidth;
                int height = embeddedTex->mHeight;
                int channels = 4; // Assimp 默认是 BGRA，每像素 4 字节
                textureID = TextureManager::get().loadTexture(texPath, raw, width, height, imageFormat);
            }
        } else {
            // 外部贴图处理
            std::string filepath = directory + '/' + texPath;
            textureID = TextureManager::get().loadTexture(filepath, imageFormat);
        }

        switch (assimpType)
        {
        case aiTextureType_DIFFUSE:{
            mat.baseColorTexture = textureID;
            break;
        }
        case aiTextureType_METALNESS:{
            mat.metallicTexture = textureID;
            break;
        }
        case aiTextureType_DIFFUSE_ROUGHNESS:{
            mat.roughnessTexture = textureID;
            break;
        }
        case aiTextureType_NORMALS:{
            mat.normalTexture = textureID;
            break;
        }
        
        default:
            break;
        }
    }

    // 获取非纹理PBR参数
    aiColor4D color;
    if (AI_SUCCESS == material->Get(AI_MATKEY_BASE_COLOR, color)) {
        mat.baseColor = glm::vec4(color.r, color.g, color.b, color.a);
    } else {
        // 尝试旧版DIFFUSE_COLOR作为回退
        if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
            mat.baseColor = glm::vec4(color.r, color.g, color.b, color.a);
        }
    }

    float value;
    if (AI_SUCCESS == material->Get(AI_MATKEY_METALLIC_FACTOR, value)) {
        mat.metallic = value;
    }
    if (AI_SUCCESS == material->Get(AI_MATKEY_ROUGHNESS_FACTOR, value)) {
        mat.roughness = value;
    }
}
