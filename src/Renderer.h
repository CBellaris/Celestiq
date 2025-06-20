#pragma once
#include "vulkan/vulkan.h"
#include "VKBase.h"
#include "Application.h"
#include "Mesh.h"
#include "Camera.h"
#include "Scene.h"
#include "Material.h"
#include <memory>

using namespace Celestiq::Vulkan;

class renderableImageAttachment {
    std::unique_ptr<imageMemory> imageMem;
    std::unique_ptr<imageView> view;
    std::unique_ptr<sampler> samp;
    std::unique_ptr<descriptorSetLayout> layout;
    std::unique_ptr<descriptorSet> set;

    std::unique_ptr<imageMemory> imageMem_old;
    std::unique_ptr<imageView> view_old;
    std::unique_ptr<descriptorSet> set_old;

public:
    bool Init(VkExtent2D extent, 
        VkFormat format, 
        VkImageUsageFlags usage, 
        VkImageAspectFlags aspect, 
        descriptorPool* pool);

    bool Resize(VkExtent2D extent,
        VkFormat format,
        VkImageUsageFlags usage,
        VkImageAspectFlags aspect,
        descriptorPool* pool);

    const VkDescriptorSetLayout GetDescriptorSetLayout() const { return layout->getHandle(); }
    const VkDescriptorSet GetDescriptorSet() const { return set->getHandle(); }
    const VkImage GetImage() const { return imageMem->Image(); }
    const VkImageView GetImageView() const { return view->getHandle(); }
    const VkImageView* GetImageViewPtr() const { return view->Address(); }
};

struct Params {
    int frameIndex = 0;
    int debugMode = 0;
    float padding1;
    float padding2;
};


class Renderer{
public:
    Renderer() = default;
    ~Renderer() = default;

    void Init();
    void update();
    void drawFrame(VkExtent2D newExtent);
    // void Cleanup();

    Params& getParams() { return r_params; }
    uint32_t getTriCount() const { return r_scene->getTriCount(); }

private:
    void resizeImageFramebuffers(VkExtent2D newExtent);
    void resizePipeline(VkExtent2D newExtent, VkCommandBuffer commandBuffer);
    void bindImageOfComputePipeline();
    // 栅栏和信号量
    std::unique_ptr<fence> r_fence;
    // std::unique_ptr<semaphore> r_semaphore_imageIsAvailable;
    // std::unique_ptr<semaphore> r_semaphore_renderingIsOver;

    std::unique_ptr<sampler> samp;

    std::unique_ptr<commandBuffer> r_commandBuffer;

    std::unique_ptr<renderPass> r_renderPass;

    std::unordered_map<std::string, std::unique_ptr<shaderModule>> r_shaders;

    std::unique_ptr<pipelineLayout> r_pipelineLayout;
    std::unique_ptr<pipeline> r_pipeline;
    std::unique_ptr<pipelineLayout> r_pipelineLayout_compute;
    std::unique_ptr<pipeline> r_pipeline_compute;

    std::unique_ptr<descriptorPool> r_descriptorPool;

    // 场景
    std::unique_ptr<Scene> r_scene;

    // 常数
    Params r_params;
    std::unique_ptr<uniformBuffer> r_constantBuffer;

    // 计算阶段 Attachments
    std::unique_ptr<renderableImageAttachment> r_computeImage_writeOnly;
    std::unique_ptr<renderableImageAttachment> r_computeImage_readOnly;
    bool r_computeImage_recreate = true;
    std::unique_ptr<descriptorSetLayout> r_descriptorSetLayout_compute;
    std::unique_ptr<descriptorSet> r_descriptorSet_compute;

    // 光照阶段 Attachments
    std::unique_ptr<renderableImageAttachment> r_onRenderImage;
    // frameBuffers
    std::unique_ptr<framebuffer> r_framebuffers;
    // 光线阶段需要采样compute pass的图像
    // std::unique_ptr<descriptorSetLayout> r_descriptorSetLayout_lighting;
    // std::unique_ptr<descriptorSet> r_descriptorSet_lighting;

    // 两个pass之间需要管线屏障
    // std::array<VkImageMemoryBarrier, 1> barriers;

    VkExtent2D extent = { 500, 500 };
    VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

    std::unique_ptr<Mesh> r_mesh_screen;

    float deltaTime = 0.0f; // 当前帧与上一帧的时间差
    float lastTime = 0.0f; // 上一帧的时间
    float currentTime = 0.0f;



    void transitionImageLayout(VkCommandBuffer cmd, const std::vector<renderableImageAttachment*>& attachments, 
                            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                            VkImageLayout oldLayout, VkImageLayout newLayout,
                            VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);

    // void transitionColorAttachmentsToShaderRead(VkCommandBuffer cmd, const std::vector<renderableImageAttachment*>& attachments);
};