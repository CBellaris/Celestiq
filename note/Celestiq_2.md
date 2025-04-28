# 搭建项目框架并绘制三角形
vulkan的话复杂是肯定的，最开始接触时很容易被其中多而杂的概念给劝退，我的建议是从一个搭建好的基础框架出发，按顺序先捋一遍所有的vulkan对象创建过程，在这个过程中先忽略大部分的参数细节，有个整体的概念。先知道为什么vulkan需要抽象出这么多的层次，之间的关系是什么，如何类比到OpenGL等，后面就会比较轻松了

这里推荐一个vulkan教程：https://easyvulkan.github.io/index.html ，这个有点类似于一个初期的中文的vulkan文档，遇到不知道有什么用的vulkan函数，懒得翻阅完整的官方英文文档，可以来这看。本节的内容你可以结合这个教程的1~3章去看

我这里的基础框架基于glfw+ImGui，窗口创建和基础UI布局的代码基本来自ImGui官方示例，在这个基础上添加自定义渲染流程，构建运行的话效果大概是这样：

<img src="assets\C2_0.png" style="zoom:50%;" />

可以拖动UI窗口改变布局，吸附窗口等，渲染视口的大小会同步改变

这章节内容会首先创建项目，然后逐步过一遍代码，简单讲解一些vulkan的内容

## 创建项目
**先下载代码存档**，工具链依旧是VSCode+MSVC，这里讲一下我自己的构建方法，你也可以使用其他编辑器。先安装MSVC，添加库glfw/imgui，如何做在**前面的教程**说过了（但不同的是这里imgui需要使用**docking分支**），我习惯是将所有的依赖库的源码直接git clone到./vendor，然后都在.tasks里面指定需要的包含路径和链接库，重点看看./.vscode/tasks.json：
```json
{
    "version": "2.0.0",
     
    "windows": {
        "options": {
            "shell": {
            "executable": "cmd.exe",
            "args": [
                "/C",
                // The path to VsDevCmd.bat depends on the version of Visual Studio you have installed.
                "vcvarsall.bat",
                "x64",
                "&&"
            ]
            }
        }
     },

    "tasks": [
        // ...
        {
            "label": "Copy res files",
            "type": "shell",
            "command": "xcopy", 
            "args": [
                "${workspaceFolder}\\res\\*", // 源路径
                "${workspaceFolder}\\build\\res\\", // 目标路径
                "/S",   // 递归复制所有子目录，包括空目录
                "/I",   // 如果目标不存在，则假定目标必须是目录
                "/Y"    // 覆盖已存在的文件而不提示
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": []
        },
        {
            "label": "build_Celestiq",
            "type": "shell",
            "command": "cl.exe",
            "args": [
                "/std:c++20",
                "/EHsc",
                "/Zi", // 需要调试时启用
                //"/Od", // 禁用优化，确保断点准确
                "/source-charset:utf-8", // 修复字符集警告
                "/Fe:${workspaceFolder}\\build\\Celestiq.exe", // 输出文件路径
                "/Fo:${workspaceFolder}\\build\\",  // 编译中间文件路径
                "/Fd:${workspaceFolder}\\build\\", // 指定.pdb文件路径
                "/I${workspaceFolder}\\App",
                "/I${workspaceFolder}\\src",
                "/I${workspaceFolder}\\vender",
                "/I${workspaceFolder}\\vender\\glfw-3.4\\include",
                "/I${workspaceFolder}\\vender\\stb_image",
                "/I${workspaceFolder}\\vender\\assimp\\include",
                "/I${workspaceFolder}\\vender\\assimp\\build\\include",
                "/I${workspaceFolder}\\vender\\json\\single_include",
                "/I${workspaceFolder}\\vender\\imgui",
                "/I${workspaceFolder}\\vender\\imgui\\backends",
                "/I${workspaceFolder}\\vender\\vulkan\\include",
                "${workspaceFolder}\\src\\*.cpp", // 源文件
                "${workspaceFolder}\\App\\*.cpp", // 源文件
                "${workspaceFolder}\\vender\\stb_image\\stb_image.cpp", // stb_image库源文件
                "${workspaceFolder}\\vender\\imgui\\*.cpp", // imgui库源文件
                "${workspaceFolder}\\vender\\imgui\\backends\\imgui_impl_glfw.cpp", // imgui库源文件
                "${workspaceFolder}\\vender\\imgui\\backends\\imgui_impl_vulkan.cpp", // imgui库源文件
                "/MDd", // 使用动态链接库(uncheck reason)
                "/link", // 链接库
                "${workspaceFolder}\\vender\\glfw-3.4\\build\\src\\Debug\\glfw3.lib", 
                "${workspaceFolder}\\vender\\assimp\\build\\lib\\Debug\\assimp-vc143-mtd.lib", 
                "${workspaceFolder}\\vender\\vulkan\\lib\\vulkan-1.lib", 
                // 必须的Window SDK库
                "gdi32.lib",
                "user32.lib",
                "shell32.lib",
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "dependsOn": "Copy res files", // 复制res文件到build
            "problemMatcher": ["$msCompile"]
        },
        // ...
    ],
    // ...
}
```
这里的options参数很重要，这样才能在命令行用MSVC编译。然后“build_Celestiq”任务用于构建本项目，构建时会先复制res文件夹到build当中，这样直接点build文件夹中的.exe就能运行渲染器了。其中详细描述了所有库所需的包含路径/源文件/链接库，可以看到glfw和assimp是需要编译的，这里还有assimp/glm/stb_image等，这章用不上，但后面肯定要用的，我这里提前安装了

然后是vulkan的安装，https://vulkan.lunarg.com/sdk/home 下载安装Windows SDK，将安装路径下的 { 你的安装路径 }/Lib/vulkan-1.lib 复制到 ./vender/vulkan/lib，安装路径下的 { 你的安装路径 }/include 全部复制到 ./vender/vulkan/include 即可，对应上面任务中的描述

./App是创建窗口和vulkan相关的底层封装的代码，而./src是渲染逻辑实际所在的代码，后面会依次介绍到

## 创建窗口
./src/main.cpp中，忽略所有关于Renderer的部分，其余的就是创建窗口的代码了。首先可以定位到`Application::Init()`，这里首先创建了glfw窗口，定义了很多的vulkan对象，在`SetupVulkan()`中，创建vulkan实例，启用调试验证层，选择物理设备，选择队列族，很多教程在这里就开始教如何查看设备对层和扩展的支持，查看所有的队列族然后做非常复杂的回退匹配机制，其实没必要，很长一段时间我们可能就启用一个调试验证层扩展，现代显卡也肯定会保证有同时支持图形、计算、呈现的队列族，一开始不考虑太多优化，设备直接选独立显卡，选一个全能队列族就可以了
```
队列族（VkQueueFamilyProperties）表示一个逻辑功能组，其可能支持以下某些操作：

VK_QUEUE_GRAPHICS_BIT: 图形管线
VK_QUEUE_COMPUTE_BIT: 通用计算
VK_QUEUE_TRANSFER_BIT: 数据传输
// 剩下几个很少用

一个物理设备可以有多个 queue family，比如：
一个支持图形 + 计算 + 呈现 的队列族
一个只支持计算的队列族，性能可能更高或在不同硬件引擎上执行

路径追踪求交、BVH 构建等与图形无关的操作就可以使用一个独立的只支持计算的队列，提高并行性能
```
然后创建了逻辑设备和创建描述池，这个描述池给ImGui专用，渲染逻辑会单独创建描述池，后面再介绍。 回到`Application::Init()`，创建Window Surface，这个由glfw为我们代劳了，不用太关心。在`SetupVulkanWindow()`中，选择了Surface Format和Present Mode，虽然是为了ImGui选的，但本渲染器不考虑支持HDR屏幕和游戏级刷新率，所以基本够用。接着调用了一个ImGui的工具函数创建了SwapChain, CommandPool, RenderPass, Framebuffer等，这里的CommandPool, RenderPass, Framebuffer是ImGui用的，我们的渲染逻辑会额外再创建，但SwapChain就用这个就行，渲染画面会作为图像递交给ImGui，让其采样显示，具体怎么上屏幕就先交给ImGui。回到`Application::Init()`，后面就都是ImGui的上下文初始化，不太需要关心了

然后可以定位到`Application::Run()`，首先可以看到layer的几个钩子的运行时机，（layer定义了imgui的不同UI层，我们这里只有主窗口中定义的一个ExampleLayer层，包括viewport和settings）`Layer::OnStart()`在vulkan实例创建完毕，主循环开始之前调用，在这里可以初始化渲染逻辑，`Layer::OnUIRender()` 在主循环的每一帧调用，在这里绘制，剩下几个目前用不上。

`Application::Run()`很大一块是为了实现ImGui的窗口停靠逻辑，不太需要关心，剩下的就是vulkan中标准的绘制一帧的流程，（交换链相关的操作都交给了ImGui，我们在渲染逻辑里就不用管这个大麻烦了）这里的绘制操作基本是给ImGui绘制UI，也是官方代码，看不懂也没关系，我们在渲染逻辑里都会再做一遍

到这里，如果删除./src/main.cpp中ExampleLayer的Renderer成员，构建就可以看到一个基本的窗口，包含空白的UI层，可以拖动、停靠和改变大小

## 插入渲染视口
将渲染逻辑插入到现在的窗口中，用到了一个很关键的函数：`ImGui::Image()`，这允许ImGui的一个UI模块直接采样图像作为显示内容，这样我们只需要将渲染内容写入图像，然后交给ImGui来显示就可以了

这里就必须先引入对一些vulkan对象的封装了，相关内容在./App/VKBase.h中，基本来自上面提到的教程当中，我只复制了用的比较多的，对vulkan对象的直接封装的类，但也有1k来行，其实都是一些重复性比较高的内容，用到了再看都可以

./src/main.cpp中的ExampleLayer，其中的renderer成员就是渲染逻辑的入口，在`Layer::OnStart()`中初始化，`Layer::OnUIRender()`中绘制帧。

### renderableImageAttachment
然后就可以看到./src/Renderer.h了，这里的renderableImageAttachment类定义了一个可以绘制并采样的对象，也就是用于渲染的图像。在vulkan中，类似于OpenGL中可以添加到帧缓冲的图像附件就是VKImageView，在SwapChain中，每个交换链图像就都是一个VKImageView。而想要采样这个图像，也就是将其用于着色器中的纹理对象，则还需要一个采样器。最后还需要将VKImageView和VkSampler合并到VkDescriptorSet，这个VkDescriptorSet就可以理解为OpenGL中的一个TextureID，可以看到我们最后将其交给ImGui时：`ImGui::Image((ImTextureID)r_onRenderImage->GetDescriptorSet(), ImVec2((float)extent.width, (float)extent.height), ImVec2(0, 1), ImVec2(1, 0));`，这里就将其转换为了ImTextureID，听上去就比较直观。

**Hint-----------**

这里就可以看到vulkan中的一个创建并使用可采样图像（例如纹理采样）的典型组合：

**(deviceMemory + image + view) ⇒ imageView**

- **VkImage**： 
表示 GPU 上的一块图像资源（2D/3D/数组等），不包含具体的内存

- **VkDeviceMemory**：
实际图像数据存放的物理内存，需要通过 vkAllocateMemory 分配，并使用 vkBindImageMemory 
与 VkImage 绑定

- **VkImageView**：
描述如何访问 VkImage 的某一部分或格式，是 Shader 中访问图像资源的句柄。（可以指定 image 的哪个 mip level、array layer、格式等）

**(imageView + sampler + descriptorSetLayout) ⇒ descriptorSet**

- **VkSampler**：
描述如何在着色器中采样图像，比如：滤波方式（线性/最近）地址模式（REPEAT/CLAMP等）mipmap 策略等

- **VkDescriptorSetLayout**：
定义 Descriptor Set 中有哪些资源（如采样图像、UBO 等），并指定其类型和绑定槽位（binding）

- **VkDescriptorSet**：
实例化后的资源绑定集合，向着色器提供真正可用的资源引用

那么这里的`renderableImageAttachment::Init()`应该就比较好看懂了，封装方法中将image+deviceMemory封装到了一起，所以创建时是比较方便的。还需要提供Resize方法，这里比较特殊的是对于旧的对象的销毁，为什么要延后一帧，以及VkDescriptorSet的销毁方法，后面会说

### DescriptorPool & DescriptorSet
单独来说一下描述符池和描述符集
描述符集（DescriptorSet）基本上就是所有GPU着色器中使用的，需要从cpu端运行时设置的数据资源（如UBO、SSBO、纹理、采样器等）的抽象。既然是抽象，那其本身肯定是不包含数据了，所以需要使用vkUpdateDescriptorSets来更新

而描述符集不能直接创建，必须从描述符池中分配，所以池是管理「一批描述符集」的内存资源，控制其生命周期和回收。池可以调用`vkDestroyDescriptorPool()`整个销毁，所有通过这个池分配的 VkDescriptorSet 都失效，这样分配和释放开销最小：不需要驱动追踪每个单独的 DescriptorSet，适合大量分配后批量释放的典型图形渲染场景（比如一整个 Frame、或者一整个 Scene）。或者在创建池时开启VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT，这样可以单独Free某个描述符集

我这里选择第二种方法，其实不算是VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT的典型应用场景，只是不想因为resize就需要重新分配所有其他的描述符集而已

### drawFrame
然后就可以在`Renderer::drawFrame()`中绘制渲染画面，首先在`Renderer::Init()`里面创建所有所需的vulkan对象，大部分都是默认值，没有什么特殊的。值得注意的是，管线中需要开启    
```cpp
pipelineCiPack.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
pipelineCiPack.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
```
动态状态来在每帧动态设置视口和裁剪大小。然后是着色器的编译，可以添加一个vs任务"Compile GLSL"来方便的编译glsl着色器代码，按照我的项目路径设置，输入着色器的文件名，就会编译./res/shader_glsl中的源码为.spv并放在./res/shader_spv中，编译项目时又会复制整个./res到./build，就能直接运行程序了

梳理一下整个渲染流程：
#### 1. 帧缓冲与图像准备
```cpp
resizeImageFramebuffers(newExtent);
```
检查当前窗口大小（newExtent）是否变化，如果变化了就需要：

重新调整渲染目标的大小（r_onRenderImage->Resize(...)）

销毁旧的 Framebuffer

创建一个新的 Framebuffer，用新的 VkImageView 作为 attachment

#### 2. 指令缓冲区录制
```cpp
r_commandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
r_renderPass->CmdBegin(r_commandBuffer->getHandle(), r_framebuffers->getHandle(),{ {}, newExtent }, makeSpanFromOne(clearColor));
resizePipeline(newExtent, r_commandBuffer->getHandle());
/*渲染命令*/
vkCmdBindPipeline(r_commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS,r_pipeline_triangle->getHandle());
vkCmdDraw(r_commandBuffer->getHandle(), 3, 1, 0, 0);
/*渲染命令结束*/
r_renderPass->CmdEnd(r_commandBuffer->getHandle());
r_commandBuffer->End();
```
这里包含三个在OpenGL中仅简化为绘制命令的三个抽象：CommandBuffer → RenderPass → Pipeline

**CommandBuffer：** 命令缓冲区是Vulkan 执行图形或计算操作的录制容器，里面包含所有 GPU 要执行的指令，比如开始渲染、绑定管线、绘制图元、切换状态等。不执行，只是录制命令。

**Render Pass：**  Vulkan 对一组 framebuffer attachments 的一次“渲染过程”的抽象，包括如何清除/加载颜色、深度等附件，何时开始/结束子通道（Subpass）等。描述整个一帧中 attachment 的生命周期

**Pipeline：** 对所有 GPU 状态的“快照”，包含着色器阶段、光栅化设置、深度/模板测试、混合等配置，是 Vulkan 中最核心的渲染配置之一

所以我们的流程就是：
```cpp
// 1. 开始录制指令缓冲区
r_commandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
```
告诉 Vulkan 开始记录命令，ONE_TIME_SUBMIT 表示这个命令缓冲区只提交一次（优化提示）

```cpp
// 2. 开始 RenderPass
r_renderPass->CmdBegin(r_commandBuffer->getHandle(), r_framebuffers->getHandle(), ...);
```
RenderPass Layout 从 UNDEFINED 转到 COLOR_ATTACHMENT_OPTIMAL

```cpp
// 3. 设置动态状态（Viewport & Scissor）
resizePipeline(newExtent, r_commandBuffer->getHandle());
```
每次绘制前用 vkCmdSetViewport 和 vkCmdSetScissor 指定viewport 和 scissor

```cpp
// 4.  绑定管线 & 发出绘制命令
vkCmdBindPipeline(r_commandBuffer->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, r_pipeline_triangle->getHandle());
vkCmdDraw(r_commandBuffer->getHandle(), 3, 1, 0, 0);
```
绘制 3 个顶点（组成一个三角形），1个 instance

#### 3. 提交执行、同步与显示
```cpp
// 提交执行
SubmitCommandBuffer_Graphics(r_commandBuffer->getHandle(), r_fence->getHandle());
```
把刚刚录制好的指令缓冲区提交到 GPU 的图形队列执行，并使用 fence 来同步：提交后 Fence 处于未完成 状态，GPU 执行完后会把它设置为完成
```cpp
// 同步与显示
extent = newExtent;
r_fence->WaitAndReset();
ImGui::Image((ImTextureID)r_onRenderImage->GetDescriptorSet(), ...);
```
CPU 等待 Fence 完成，保证 GPU 已经完成这一帧渲染

重置 Fence，以便下一帧可以继续用

通过 ImGui 把渲染好的图像显示到 ImGui 的 GUI 界面上

## 构建
至此整个框架应该比较清晰了，还顺带讲了一些vulkan概念，仅为个人理解，严谨一些的内容还是去别的教程看，不过整个看下来的话，对初期的学习应该是有帮助的

下载代码存档后，还需要额外处理库依赖才能构建，我个人不太喜欢把库到处复制和下载预编译的库文件，所以教程里这块写的比较详细了。运行程序会是这样：

<img src="assets\C2_1.png" style="zoom:50%;" />

因为没设置默认布局，拖动左上角叠在一起的窗口调整，配置会被写在imgui.ini，窗口位置就被保存到下次启动了

<img src="assets\C2_2.png" style="zoom:50%;" />