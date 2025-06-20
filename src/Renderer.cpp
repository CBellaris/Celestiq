#include "Renderer.h"
#include <GLFW/glfw3.h>
#include "Input.h"
#include "GlobalSettings.h"

bool renderableImageAttachment::Init(VkExtent2D extent, 
                                    VkFormat format, 
                                    VkImageUsageFlags usage, 
                                    VkImageAspectFlags aspect, 
                                    descriptorPool* pool)
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
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 500 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 }
    };
    r_descriptorPool = std::make_unique<descriptorPool>(100, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT);

    r_commandBuffer = std::make_unique<commandBuffer>();
    Celestiq::Application::Get().GetCommandPoolGraphics()->AllocateBuffers(makeSpanFromOne(r_commandBuffer.get()));

    // 栅栏
    r_fence = std::make_unique<fence>();
    // r_semaphore_imageIsAvailable = std::make_unique<semaphore>();
    // r_semaphore_renderingIsOver = std::make_unique<semaphore>();

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

    r_params = Params{};
    r_params.frameIndex = 0;
    r_constantBuffer = std::make_unique<uniformBuffer>();
    r_constantBuffer->Create(sizeof(Params));

    // 光照阶段用 RenderPass----------------------------------------------------------------------------------------------------
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
    dependency.srcStageMask  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkRenderPassCreateInfo rpInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments    = &colorAttachment;
    rpInfo.subpassCount    = 1;
    rpInfo.pSubpasses      = &subpass;
    rpInfo.dependencyCount = 1;
    rpInfo.pDependencies   = &dependency;

    r_renderPass = std::make_unique<renderPass>(rpInfo);

    // compute阶段图像附件----------------------------------------------------------
    r_computeImage_writeOnly = std::make_unique<renderableImageAttachment>();
    r_computeImage_writeOnly->Init(extent,
                             VK_FORMAT_R32G32B32A32_SFLOAT,
                             VK_IMAGE_USAGE_STORAGE_BIT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             r_descriptorPool.get());

    r_computeImage_readOnly = std::make_unique<renderableImageAttachment>();
    r_computeImage_readOnly->Init(extent,
                             VK_FORMAT_R32G32B32A32_SFLOAT,
                             VK_IMAGE_USAGE_STORAGE_BIT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             r_descriptorPool.get());

    // 光照阶段图像附件及Framebuffer----------------------------------------------------------
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

    // 场景
    r_scene = std::make_unique<Scene>();
    r_scene->initScene();
    r_scene->initDescriptor(r_descriptorPool.get());
    r_scene->writeDescriptor();

    r_scene->getCamera()->addCameraUpdateCallback([this](){
        r_params.frameIndex = 0;
    });

    // 图像纹理
    TextureManager::get().initDescriptorSet(r_descriptorPool.get());

    // 创建compute管线 ---------------------------------------------------------------------------------
    // 先创建存储图像附件的描述符集布局和描述符集
    r_descriptorSetLayout_compute = std::make_unique<descriptorSetLayout>();
    r_descriptorSet_compute = std::make_unique<descriptorSet>();
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_compute[3] ={
        // 存储图像WriteOnly
        {
            .binding = 0,                                       //描述符被绑定到0号binding
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  //类型为存储图像
            .descriptorCount = 1,                               //个数是1个
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT            //在计算着色器阶段读取存储图像
        },
        // 存储图像ReadOnly
        {
            .binding = 1,                                       //描述符被绑定到1号binding
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  //类型为存储图像
            .descriptorCount = 1,                               //个数是1个
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT            //在计算着色器阶段读取存储图像
        },
        // 常量缓冲区
        {
            .binding = 2,                                       //描述符被绑定到2号binding
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  //类型为UBO
            .descriptorCount = 1,                               //个数是1个
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT            //在计算着色器阶段读取存储图像
        }

    };
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo_compute = {
        .bindingCount = 3,
        .pBindings = descriptorSetLayoutBinding_compute
    };
    r_descriptorSetLayout_compute->Create(descriptorSetLayoutCreateInfo_compute);
    r_descriptorPool->AllocateSets(makeSpanFromOne(r_descriptorSet_compute.get()), makeSpanFromOne(r_descriptorSetLayout_compute.get()));
    // 存储图像
    VkDescriptorImageInfo imageInfo_writeOnly = {
        .imageView = r_computeImage_writeOnly->GetImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };
    VkDescriptorImageInfo imageInfo_readOnly = {
        .imageView = r_computeImage_readOnly->GetImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };
    // 常量缓冲区
    VkDescriptorBufferInfo bufferInfo_camera = {
        .buffer = r_constantBuffer->getHandle(),
        .offset = 0,
        .range = sizeof(Params)
    };
    r_descriptorSet_compute->Write(makeSpanFromOne(imageInfo_writeOnly), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0);
    r_descriptorSet_compute->Write(makeSpanFromOne(imageInfo_readOnly), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1);
    r_descriptorSet_compute->Write(makeSpanFromOne(bufferInfo_camera), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2);

    // 将场景、图像纹理和存储图像的描述符集布局合并
    VkDescriptorSetLayout layouts[3] = {
        r_scene->getDescriptorSetLayout(),
        TextureManager::get().getDescriptorSetLayout(),
        r_descriptorSetLayout_compute->getHandle()
    };
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo_compute = {
        .setLayoutCount = 3,
        .pSetLayouts = layouts
    };
    r_pipelineLayout_compute = std::make_unique<pipelineLayout>(pipelineLayoutCreateInfo_compute);

    // 创建着色器
    r_shaders["compute"] = std::make_unique<shaderModule>("res/shader_spv/Deferred_C.comp.spv");
    VkPipelineShaderStageCreateInfo shaderStageCreateInfos_compute[1] = {
        r_shaders["compute"]->StageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT)
    };

    VkComputePipelineCreateInfo computePipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = shaderStageCreateInfos_compute[0],
        .layout = r_pipelineLayout_compute->getHandle()
    };
    r_pipeline_compute = std::make_unique<pipeline>(computePipelineCreateInfo);

    // 创建光照管线 ---------------------------------------------------------------------------------------
    // 绑定计算阶段的存储图像的描述符布局
    VkDescriptorSetLayout r_computeImage_layout = r_computeImage_writeOnly->GetDescriptorSetLayout();
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &r_computeImage_layout
    };

    r_pipelineLayout = std::make_unique<pipelineLayout>(pipelineLayoutInfo);

    // 创建着色器
    r_shaders["vert"] = std::make_unique<shaderModule>("res/shader_spv/Deferred_L.vert.spv");
    r_shaders["frag"] = std::make_unique<shaderModule>("res/shader_spv/Deferred_L.frag.spv");
    VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2] = {
        r_shaders["vert"]->StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
        r_shaders["frag"]->StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    graphicsPipelineCreateInfoPack pipelineCiPack;
    pipelineCiPack.createInfo.layout = r_pipelineLayout->getHandle();
    pipelineCiPack.createInfo.renderPass = r_renderPass->getHandle();
    pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineCiPack.rasterizationStateCi.lineWidth = 1.0f;
    pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineCiPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });
    pipelineCiPack.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    pipelineCiPack.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    pipelineCiPack.UpdateAllArrays();
    pipelineCiPack.createInfo.stageCount = 2;
    pipelineCiPack.createInfo.pStages = shaderStageCreateInfos;
    Mesh::bind_pipeline(pipelineCiPack);
    r_pipeline = std::make_unique<pipeline>(pipelineCiPack);
// -----------------------------------------------------------------------------------------

    r_mesh_screen = std::make_unique<Mesh>();
    r_mesh_screen->set_mesh_screen();
    r_mesh_screen->create_vertex_buffer();
}

void Renderer::update()
{
    // Debug模式切换
    if (Celestiq::Input::IsKeyDown(Celestiq::KeyCode::D0)) {
        r_params.debugMode = 0;
        r_params.frameIndex = 0; // 切换到Debug模式时重置帧索引
    }
    if (Celestiq::Input::IsKeyDown(Celestiq::KeyCode::D1)) {
        r_params.debugMode = 1;
    }
    if (Celestiq::Input::IsKeyDown(Celestiq::KeyCode::D2)) {
        r_params.debugMode = 2;
    }
    if (Celestiq::Input::IsKeyDown(Celestiq::KeyCode::D3)) {
        r_params.debugMode = 3;
    }
    if (Celestiq::Input::IsKeyDown(Celestiq::KeyCode::D4)) {
        r_params.debugMode = 4;
    }
    if (Celestiq::Input::IsKeyDown(Celestiq::KeyCode::D5)) {
        r_params.debugMode = 5;
    }
    if (Celestiq::Input::IsKeyDown(Celestiq::KeyCode::D6)) {
        r_params.debugMode = 6;
    }
    if (Celestiq::Input::IsKeyDown(Celestiq::KeyCode::D7)) {
        r_params.debugMode = 7;
    }
}

void Renderer::drawFrame(VkExtent2D newExtent)
{
    // 计算帧时间
    currentTime = glfwGetTime();
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    // 更新场景
    r_scene->update(deltaTime);
    // 处理窗口相关输入
    Celestiq::Input::getInstance().update();
    // 处理渲染器相关输入
    this->update();

    // 更新常量缓冲区
    r_constantBuffer->TransferData(&r_params, sizeof(Params));

    resizeImageFramebuffers(newExtent);

    // 开始命令录入
    r_commandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    // Compute Pass-----------------------------------------------------------------
    if(r_computeImage_recreate)
    {
        // VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_GENERAL
        transitionImageLayout(r_commandBuffer->getHandle(), {r_computeImage_writeOnly.get(), r_computeImage_readOnly.get()},
                        0, VK_ACCESS_SHADER_READ_BIT,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        r_computeImage_recreate = false;
    } else{
        // 将着色器可读图像 转换为 存储图像
        transitionImageLayout(r_commandBuffer->getHandle(), {r_computeImage_writeOnly.get()},
                        VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        transitionImageLayout(r_commandBuffer->getHandle(), {r_computeImage_readOnly.get()},
                        VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    }
    vkCmdBindPipeline(r_commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, r_pipeline_compute->getHandle());
    VkDescriptorSet sets[3] = {
        r_scene->getDescriptorSet(),
        TextureManager::get().getDescriptorSet(),
        r_descriptorSet_compute->getHandle()
    };
    vkCmdBindDescriptorSets(r_commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, r_pipelineLayout_compute->getHandle(),
                            0, 3, sets, 0, nullptr);
    vkCmdDispatch(r_commandBuffer->getHandle(), ceil(newExtent.width/16), ceil(newExtent.height/16), 1);
    // 将存储图像 转换为 着色器可读图像
    transitionImageLayout(r_commandBuffer->getHandle(), {r_computeImage_writeOnly.get(), r_computeImage_readOnly.get()},
                        VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);


    // 光照pass------------------------------------------------------------------------
    VkClearValue clearColor = { .color = { 0.0f, 0.0f, 0.0f, 1.f } };
    r_renderPass->CmdBegin(r_commandBuffer->getHandle(), r_framebuffers->getHandle(), { {}, newExtent }, makeSpanFromOne(clearColor));
    resizePipeline(newExtent, r_commandBuffer->getHandle());
    /*渲染命令*/
    VkDescriptorSet r_computeImage_descriptorSet = r_computeImage_writeOnly->GetDescriptorSet();
    vkCmdBindDescriptorSets(r_commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS,
        r_pipelineLayout->getHandle(), 0, 1, &r_computeImage_descriptorSet, 0, nullptr);
    vkCmdBindPipeline(r_commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, r_pipeline->getHandle());
    r_mesh_screen->draw(r_commandBuffer->getHandle());
    /*渲染命令结束*/
    r_renderPass->CmdEnd(r_commandBuffer->getHandle());

    // 结束命令录入
    r_commandBuffer->End();
    SubmitCommandBuffer_Graphics(r_commandBuffer->getHandle(), r_fence->getHandle());

    // 等待绘制命令执行完成
    extent = newExtent;
    r_fence->WaitAndReset();
    r_params.frameIndex++;

    // 计算着色器中的双图像交换
    std::swap(r_computeImage_writeOnly, r_computeImage_readOnly);
    bindImageOfComputePipeline();

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

    r_computeImage_writeOnly->Resize(newExtent,
                                  VK_FORMAT_R32G32B32A32_SFLOAT,
                                  VK_IMAGE_USAGE_STORAGE_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  r_descriptorPool.get());

    r_computeImage_readOnly->Resize(newExtent,
                                  VK_FORMAT_R32G32B32A32_SFLOAT,
                                  VK_IMAGE_USAGE_STORAGE_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  r_descriptorPool.get());

    r_computeImage_recreate = true;

    // r_computeImage重新创建了，需要重新更新描述符集
    bindImageOfComputePipeline();
    
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

void Renderer::bindImageOfComputePipeline()
{
    // 绑定存储图像的描述符集到计算管线
    VkDescriptorImageInfo imageInfo_writeOnly = {
        .imageView = r_computeImage_writeOnly->GetImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };
    VkDescriptorImageInfo imageInfo_readOnly = {
        .imageView = r_computeImage_readOnly->GetImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };
    r_descriptorSet_compute->Write(makeSpanFromOne(imageInfo_writeOnly), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0);
    r_descriptorSet_compute->Write(makeSpanFromOne(imageInfo_readOnly), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1);
}

void Renderer::transitionImageLayout(VkCommandBuffer cmd, const std::vector<renderableImageAttachment*>& attachments, 
                                    VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                                    VkImageLayout oldLayout, VkImageLayout newLayout,
                                    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
    std::vector<VkImageMemoryBarrier> barriers;
    barriers.reserve(attachments.size());

    for (const auto* attachment : attachments)
    {
        VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;
        barrier.oldLayout     = oldLayout;
        barrier.newLayout     = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = attachment->GetImage();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barriers.push_back(barrier);
    }

    vkCmdPipelineBarrier(cmd,
                        srcStageMask,
                        dstStageMask,
                        0,
                        0, nullptr,
                        0, nullptr,
                        static_cast<uint32_t>(barriers.size()), barriers.data());
}