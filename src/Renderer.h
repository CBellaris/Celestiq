#include "vulkan/vulkan.h"
#include "VKBase.h"
#include "Application.h"
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

    const VkDescriptorSet GetDescriptorSet() const { return set->getHandle(); }
    const VkImageView GetImageView() const { return view->getHandle(); }
    const VkImageView* GetImageViewPtr() const { return view->Address(); }
};


class Renderer{
public:
    Renderer() = default;
    ~Renderer() = default;

    void Init();
    void drawFrame(VkExtent2D newExtent);
    // void Cleanup();

private:
    void resizeImageFramebuffers(VkExtent2D newExtent);
    void resizePipeline(VkExtent2D newExtent, VkCommandBuffer commandBuffer);
    // 栅栏和信号量
    std::unique_ptr<fence> r_fence;
    // std::unique_ptr<semaphore> r_semaphore_imageIsAvailable;
    // std::unique_ptr<semaphore> r_semaphore_renderingIsOver;

    std::unique_ptr<commandBuffer> r_commandBuffer;
    std::unique_ptr<commandPool> r_commandPool;
    std::unique_ptr<renderPass> r_renderPass;
    std::unordered_map<std::string, std::unique_ptr<shaderModule>> r_shaders;
    std::unique_ptr<pipelineLayout> r_pipelineLayout_triangle;
    std::unique_ptr<pipeline> r_pipeline_triangle;

    std::unique_ptr<descriptorPool> r_descriptorPool;
    std::unique_ptr<renderableImageAttachment> r_onRenderImage;
    std::unique_ptr<framebuffer> r_framebuffers;

    VkExtent2D extent = { 500, 500 };
    VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

public:
    //该函数用于将命令缓冲区提交到用于图形的队列
    static VkResult SubmitCommandBuffer_Graphics(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE);
    //该函数用于在渲染循环中将命令缓冲区提交到图形队列的常见情形
    static VkResult SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer,
        VkSemaphore semaphore_imageIsAvailable = VK_NULL_HANDLE, VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE,
        VkPipelineStageFlags waitDstStage_imageIsAvailable = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    static VkResult SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE);
};