#include "Renderer.h"

bool renderableImageAttachment::Init(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, descriptorPool* pool)
{
    // Create VkImage
    VkImageCreateInfo imageCreateInfo = {
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = { extent.width, extent.height, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT,  // Important!
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    imageMem = std::make_unique<imageMemory>(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Create image view
    VkImageSubresourceRange range = {
        .aspectMask = aspect,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
    };
    view = std::make_unique<imageView>(imageMem->Image(), VK_IMAGE_VIEW_TYPE_2D, format, range);

    // 配置采样器
    VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter    = VK_FILTER_LINEAR;
    samplerInfo.minFilter    = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod       = 1.0f;
    // Create sampler
    samp = std::make_unique<sampler>(samplerInfo);

    // Descriptor layout (binding 0: combined image sampler)
    VkDescriptorSetLayoutBinding binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .bindingCount = 1,
        .pBindings = &binding
    };
    layout = std::make_unique<descriptorSetLayout>(layoutInfo);

    set = std::make_unique<descriptorSet>();
    // Allocate descriptor set
    if (pool->AllocateSets(makeSpanFromOne(set.get()), makeSpanFromOne(layout.get()))) return false;

    // Update descriptor
    VkDescriptorImageInfo imgInfo = {
        .sampler = samp->getHandle(),
        .imageView = view->getHandle(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    set->Write({ &imgInfo, 1 }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    return true;
}

bool renderableImageAttachment::Resize(VkExtent2D extent,
                                       VkFormat format,
                                       VkImageUsageFlags usage,
                                       VkImageAspectFlags aspect,
                                       descriptorPool* pool)
{
    // 这里延后一帧销毁旧的 set + imageview 不然ImGui::Image 会依旧持有旧的描述符集，无法释放
    if(set_old) {
        pool->FreeSets(makeSpanFromOne(set_old.get())); // 先释放描述符集，否则图像视图会被引用计数器保护，无法释放
        set_old.reset();
    }
    if(view_old) view_old.reset();
    if(imageMem_old) imageMem_old.reset();

    // 重建 VkImage
    VkImageCreateInfo imageCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format        = format;
    imageCreateInfo.extent        = { extent.width, extent.height, 1 };
    imageCreateInfo.mipLevels     = 1;
    imageCreateInfo.arrayLayers   = 1;
    imageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage         = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    std::unique_ptr<imageMemory> imageMem_new = std::make_unique<imageMemory>(imageCreateInfo,
                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // 重建 ImageView
    VkImageSubresourceRange range{
        .aspectMask     = aspect,
        .baseMipLevel   = 0,
        .levelCount     = 1,
        .baseArrayLayer = 0,
        .layerCount     = 1
    };
    std::unique_ptr<imageView> view_new = std::make_unique<imageView>(imageMem_new->Image(),
                                       VK_IMAGE_VIEW_TYPE_2D,
                                       format,
                                       range);


    // 重建描述符集
    std::unique_ptr<descriptorSet> set_new = std::make_unique<descriptorSet>();
    // Allocate descriptor set
    if (pool->AllocateSets(makeSpanFromOne(set_new.get()), makeSpanFromOne(layout.get()))) return false;

    VkDescriptorImageInfo imgInfo{
        .sampler     = samp->getHandle(),
        .imageView   = view_new->getHandle(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    set_new->Write({ &imgInfo, 1 },
               VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // 保存到后一帧销毁
    view_old = std::move(view);
    imageMem_old = std::move(imageMem);
    set_old = std::move(set);

    view = std::move(view_new);
    imageMem = std::move(imageMem_new);
    set = std::move(set_new);

    return true;
}


void Renderer::Init()
{
    // 创建描述符池
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 }
    };
    r_descriptorPool = std::make_unique<descriptorPool>(100, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

    r_commandBuffer = std::make_unique<commandBuffer>();
    r_commandPool = std::make_unique<commandPool>(Celestiq::Application::GetQueueFamilyIndex(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    r_commandPool->AllocateBuffers(makeSpanFromOne(r_commandBuffer.get()));

    // 栅栏
    r_fence = std::make_unique<fence>();
    // r_semaphore_imageIsAvailable = std::make_unique<semaphore>();
    // r_semaphore_renderingIsOver = std::make_unique<semaphore>();

    // 离屏渲染用 RenderPass
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = imageFormat;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    // 这里把 finalLayout 改成 SHADER_READ_ONLY，以便后面 ImGui::Image 能直接采样
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo rpInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments    = &colorAttachment;
    rpInfo.subpassCount    = 1;
    rpInfo.pSubpasses      = &subpass;
    rpInfo.dependencyCount = 1;
    rpInfo.pDependencies   = &dependency;

    r_renderPass = std::make_unique<renderPass>(rpInfo);


    r_onRenderImage = std::make_unique<renderableImageAttachment>();
    if (!r_onRenderImage->Init(extent,
                               imageFormat,
                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                               VK_IMAGE_ASPECT_COLOR_BIT,         
                               r_descriptorPool.get())) {
        throw std::runtime_error("Failed to init renderableImageAttachment");
    }


    VkImageView attachment = r_onRenderImage->GetImageView();
    VkFramebufferCreateInfo fbInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    fbInfo.renderPass      = r_renderPass->getHandle();
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments    = &attachment;
    fbInfo.width           = extent.width;
    fbInfo.height          = extent.height;
    fbInfo.layers          = 1;

    r_framebuffers = std::make_unique<framebuffer>();
    r_framebuffers->Create(fbInfo);


    // 创建管线
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    r_pipelineLayout_triangle = std::make_unique<pipelineLayout>(pipelineLayoutCreateInfo);
    r_shaders["vert"] = std::make_unique<shaderModule>("res/shader_spv/FirstTriangle.vert.spv");
    r_shaders["frag"] = std::make_unique<shaderModule>("res/shader_spv/FirstTriangle.frag.spv");
    static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_triangle[2] = {
        r_shaders["vert"]->StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
        r_shaders["frag"]->StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    graphicsPipelineCreateInfoPack pipelineCiPack;
    pipelineCiPack.createInfo.layout = r_pipelineLayout_triangle->getHandle();
    pipelineCiPack.createInfo.renderPass = r_renderPass->getHandle();
    pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineCiPack.rasterizationStateCi.lineWidth = 1.0f;
    pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineCiPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });
    pipelineCiPack.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    pipelineCiPack.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    pipelineCiPack.UpdateAllArrays();
    pipelineCiPack.createInfo.stageCount = 2;
    pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_triangle;
    r_pipeline_triangle = std::make_unique<pipeline>(pipelineCiPack);

}

void Renderer::drawFrame(VkExtent2D newExtent)
{
    resizeImageFramebuffers(newExtent);
    VkClearValue clearColor = { .color = { 0.2f, 0.2f, 0.2f, 1.f } };

    r_commandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    r_renderPass->CmdBegin(r_commandBuffer->getHandle(), r_framebuffers->getHandle(), { {}, newExtent }, makeSpanFromOne(clearColor));
    resizePipeline(newExtent, r_commandBuffer->getHandle());
    /*渲染命令*/
    vkCmdBindPipeline(r_commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, r_pipeline_triangle->getHandle());
    vkCmdDraw(r_commandBuffer->getHandle(), 3, 1, 0, 0);
    /*渲染命令结束*/
    r_renderPass->CmdEnd(r_commandBuffer->getHandle());
    r_commandBuffer->End();

    SubmitCommandBuffer_Graphics(r_commandBuffer->getHandle(), r_fence->getHandle());

    extent = newExtent;
    r_fence->WaitAndReset();
    ImGui::Image((ImTextureID)r_onRenderImage->GetDescriptorSet(), ImVec2((float)extent.width, (float)extent.height), ImVec2(0, 1), ImVec2(1, 0));
}

void Renderer::resizeImageFramebuffers(VkExtent2D newExtent)
{
    // 零尺寸保护
    if (newExtent.width == 0 || newExtent.height == 0){
        std::cout << "Renderer::resizeImageFramebuffers: zero size" << std::endl;
        return;
    }
    // 无变化直接返回
    if (newExtent.width == extent.width && newExtent.height == extent.height)
        return;

    r_onRenderImage->Resize(newExtent,
                            imageFormat,
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                            VK_IMAGE_ASPECT_COLOR_BIT,
                            r_descriptorPool.get());
    
    // 销毁旧的 Framebuffer
    r_framebuffers.reset();
    // 用新 Attachment 的 ImageView 创建 Framebuffer
    VkImageView attachment = r_onRenderImage->GetImageView();
    VkFramebufferCreateInfo fbInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    fbInfo.renderPass      = r_renderPass->getHandle();
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments    = &attachment;
    fbInfo.width           = newExtent.width;
    fbInfo.height          = newExtent.height;
    fbInfo.layers          = 1;

    r_framebuffers = std::make_unique<framebuffer>();
    r_framebuffers->Create(fbInfo);
}

void Renderer::resizePipeline(VkExtent2D newExtent, VkCommandBuffer commandBuffer)
{
    // 零尺寸保护
    if (newExtent.width == 0 || newExtent.height == 0){
        std::cout << "Renderer::resizePipeline: zero size" << std::endl;
        return;
    }
    // 每帧都需要设置 viewport 和 scissor，不管有没有变化
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = static_cast<float>(newExtent.width);
    viewport.height = static_cast<float>(newExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = newExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

VkResult Renderer::SubmitCommandBuffer_Graphics(VkSubmitInfo &submitInfo, VkFence fence)
{
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkResult result = vkQueueSubmit(Celestiq::Application::GetQueue(), 1, &submitInfo, fence);
    if (result)
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to submit the command buffer!\nError code: {}\n", int32_t(result));
    return result;
}

VkResult Renderer::SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer, VkSemaphore semaphore_imageIsAvailable, VkSemaphore semaphore_renderingIsOver, VkFence fence, VkPipelineStageFlags waitDstStage_imageIsAvailable)
{
    VkSubmitInfo submitInfo = {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };
    if (semaphore_imageIsAvailable)
        submitInfo.waitSemaphoreCount = 1,
        submitInfo.pWaitSemaphores = &semaphore_imageIsAvailable,
        submitInfo.pWaitDstStageMask = &waitDstStage_imageIsAvailable;
    if (semaphore_renderingIsOver)
        submitInfo.signalSemaphoreCount = 1,
        submitInfo.pSignalSemaphores = &semaphore_renderingIsOver;
    return SubmitCommandBuffer_Graphics(submitInfo, fence);
}

VkResult Renderer::SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer, VkFence fence)
{
    VkSubmitInfo submitInfo = {
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };
    return SubmitCommandBuffer_Graphics(submitInfo, fence);
}
