#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include "GlobalSettings.h"
#include "VKBase.h"


struct alignas(16) MaterialData {
    alignas(16) glm::vec3 baseColor = glm::vec3(0.5f);
    alignas(16) glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);  
    float metallic        = 0.0f;
    float roughness       = 0.5f;
    float ior             = 1.5f;     // 折射率，空气≈1.0，玻璃≈1.5
    float transmission    = 0.0f;     // 透射能量占比（“透明度”）

    int baseColorTexture    = -1;        // -1 表示无纹理
    int normalTexture       = -1;
    int metallicTexture     = -1;
    int roughnessTexture    = -1;           
    int transmissionTexture = -1;     
};

inline bool operator==(const MaterialData& a, const MaterialData& b) {
    return a.baseColor == b.baseColor &&
           a.normal == b.normal &&
           a.metallic == b.metallic &&
           a.roughness == b.roughness &&
           a.ior == b.ior && 
           a.transmission == b.transmission &&
           a.baseColorTexture == b.baseColorTexture &&
           a.normalTexture == b.normalTexture &&
           a.metallicTexture == b.metallicTexture &&
           a.roughnessTexture == b.roughnessTexture &&
           a.transmissionTexture == b.transmissionTexture;
}

// 自定义哈希函数
namespace std {
    template <>
    struct hash<MaterialData> {
        std::size_t operator()(const MaterialData& mat) const {
            size_t h = 0;
            auto hash_combine = [&](auto val) {
                h ^= std::hash<decltype(val)>{}(val) + 0x9e3779b9 + (h << 6) + (h >> 2);
            };
            hash_combine(mat.baseColor.x);
            hash_combine(mat.baseColor.y);
            hash_combine(mat.baseColor.z);
            hash_combine(mat.roughness);
            hash_combine(mat.normal.x);
            hash_combine(mat.normal.y);
            hash_combine(mat.normal.z);
            hash_combine(mat.metallic);
            hash_combine(mat.baseColorTexture);
            hash_combine(mat.normalTexture);
            hash_combine(mat.metallicTexture);
            hash_combine(mat.roughnessTexture);
            return h;
        }
    };
}

// 管理单个MaterialData，先不支持图像纹理功能，创建时拥有唯一id。不提供外部创建方法，由MaterialManager创建
class Material {
    friend class MaterialManager;

public:
    uint32_t getID() const { return m_id; }
    const MaterialData& getData() const { return m_data; }
    MaterialData& getMutableData() { return m_data; }

private:
    Material(uint32_t id, const MaterialData& data) : m_id(id), m_data(data) {}

    uint32_t m_id;
    MaterialData m_data;
};


// 全局的材质管理，单例类，提供单个材质的创建，通过id索引并返回单个材质
class MaterialManager {
public:
    static MaterialManager& get() {
        static MaterialManager instance;
        return instance;
    }

    // 创建一个新材质，返回其ID
    uint32_t createMaterial(const MaterialData& data = MaterialData()) {
        auto it = m_dataToID.find(data);
        if (it != m_dataToID.end())
            return it->second;

        uint32_t id = static_cast<uint32_t>(m_materials.size());
        m_materials.push_back(Material(id, data));
        m_dataToID[data] = id;
        return id;
    }

    // 获取材质引用（只读）
    const Material& getMaterial(uint32_t id) const {
        assert(id < m_materials.size());
        return m_materials[id];
    }

    // 获取材质引用（可修改）
    Material& getMutableMaterial(uint32_t id) {
        assert(id < m_materials.size());
        return m_materials[id];
    }

    // 返回所需SSBO大小
    uint64_t getMaterialDataSize() {
        return m_materials.size() * sizeof(MaterialData);
    }

    // 将材质数据打包为GPU上传格式
    std::vector<MaterialData> getMaterialDataBuffer() const {
        std::vector<MaterialData> buffer;
        buffer.reserve(m_materials.size());
        for (const auto& mat : m_materials)
            buffer.push_back(mat.getData());
        return buffer;
    }

private:
    std::vector<Material> m_materials;
    std::unordered_map<MaterialData, uint32_t> m_dataToID;  // 去重索引

    // 禁止外部创建
    MaterialManager() = default;
};

class TextureManager {
public:
    static TextureManager& get() {
        static TextureManager instance;
        return instance;
    }

    static VkSampler getSampler() {
        VkSamplerCreateInfo samplerInfo = Celestiq::Vulkan::texture::SamplerCreateInfo();
        static std::unique_ptr<Celestiq::Vulkan::sampler> sampler = std::make_unique<Celestiq::Vulkan::sampler>(samplerInfo);

        return sampler->getHandle();
    }

    // 加载纹理（路径去重），返回绑定索引
    uint32_t loadTexture(const std::string& path, VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB) {
        auto it = m_pathToIndex.find(path);
        if (it != m_pathToIndex.end()) return it->second;

        uint32_t index = static_cast<uint32_t>(m_textures.size());
        std::unique_ptr<Celestiq::Vulkan::texture2d> texture = std::make_unique<Celestiq::Vulkan::texture2d>(path.c_str(), imageFormat, imageFormat, false);
        m_textures.push_back(std::move(texture));
        m_pathToIndex[path] = index;
        return index;
    }
    uint32_t loadTexture(const std::string& path, const uint8_t* rawData, size_t fileSize, VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB) {
        auto it = m_pathToIndex.find(path);
        if (it != m_pathToIndex.end()) return it->second;

        uint32_t index = static_cast<uint32_t>(m_textures.size());
        std::unique_ptr<Celestiq::Vulkan::texture2d> texture = std::make_unique<Celestiq::Vulkan::texture2d>(rawData, fileSize, imageFormat, imageFormat, false);
        m_textures.push_back(std::move(texture));
        m_pathToIndex[path] = index;
        return index;
    }
    uint32_t loadTexture(const std::string& path, const uint8_t* rawData, unsigned int width, unsigned int height, VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB) {
        auto it = m_pathToIndex.find(path);
        if (it != m_pathToIndex.end()) return it->second;

        uint32_t index = static_cast<uint32_t>(m_textures.size());
        std::unique_ptr<Celestiq::Vulkan::texture2d> texture = std::make_unique<Celestiq::Vulkan::texture2d>(rawData, VkExtent2D {width, height}, imageFormat, imageFormat, false);
        m_textures.push_back(std::move(texture));
        m_pathToIndex[path] = index;
        return index;
    }

    // 获取所有 image view 和 sampler，供写入描述符数组
    void getDescriptorImageInfos(std::vector<VkDescriptorImageInfo>& outInfos) const {
        outInfos.clear();
        outInfos.reserve(m_textures.size());
        for (const auto& tex : m_textures) {
            outInfos.push_back(VkDescriptorImageInfo{
                .sampler = getSampler(),
                .imageView = tex->ImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });
        }
    }

    void initDescriptorSet(Celestiq::Vulkan::descriptorPool* pool) {
        VkDescriptorSetLayoutBinding textureArrayBinding{};
        textureArrayBinding.binding = 0;
        textureArrayBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureArrayBinding.descriptorCount = static_cast<uint32_t>(GlobalSettings::TempSetting::MAX_TEXTURE_COUNT);  // 比如 1024
        textureArrayBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        textureArrayBinding.pImmutableSamplers = nullptr;

        // 额外启用 descriptor indexing flags
        VkDescriptorBindingFlagsEXT bindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
                                                   VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;

        VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
            .bindingCount = 1,
            .pBindingFlags = &bindingFlags
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &textureArrayBinding;
        layoutInfo.pNext = &bindingFlagsInfo;  // 把扩展信息挂进去
        layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

        m_descriptorSetLayout = std::make_unique<Celestiq::Vulkan::descriptorSetLayout>(layoutInfo);
        m_descriptorSet = std::make_unique<Celestiq::Vulkan::descriptorSet>();
        pool->AllocateSets(Celestiq::Vulkan::makeSpanFromOne(m_descriptorSet.get()), makeSpanFromOne(m_descriptorSetLayout.get()));

        std::vector<VkDescriptorImageInfo> imageInfos;
        getDescriptorImageInfos(imageInfos);

        m_descriptorSet->Write(imageInfos, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    size_t getTextureCount() const { return m_textures.size(); }
    VkDescriptorSet getDescriptorSet() const { return m_descriptorSet->getHandle(); }
    VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout->getHandle(); }

private:
    std::vector<std::unique_ptr<Celestiq::Vulkan::texture2d>> m_textures;
    std::unordered_map<std::string, int32_t> m_pathToIndex;
    std::unique_ptr<Celestiq::Vulkan::descriptorSetLayout> m_descriptorSetLayout;
    std::unique_ptr<Celestiq::Vulkan::descriptorSet> m_descriptorSet;

    TextureManager() = default;
};
