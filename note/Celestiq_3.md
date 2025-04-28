# 添加基础功能

## Buffer Object
缓冲对象在vulkan中算是个大话题，这里先简单总结一下

### 顶点数据
在话题切入到Buffer Object之前，先看看其能用在哪。以顶点数据，也就是vertex buffer为例，这里基本可以类比OpenGL中的VBO+VBOLayout = VAO, 绘制：glBindVertexArray(VAO)，不同的是，vulkan将VAO放在了Pipeline中，对应关系大概是：

| OpenGL              | Vulkan                               | 解释                   |
|:-------------------|:-------------------------------------|:-----------------------|
| VBO (缓冲区)         | `VkBuffer` (vertex buffer)           | 存储顶点数据的缓冲区 |
| VAO (顶点数组对象)  | `VkPipelineVertexInputStateCreateInfo` | 记录顶点数据格式和布局 |
| VBO Layout          | `VkVertexInputBindingDescription` + `VkVertexInputAttributeDescription` | 分别描述绑定和每个属性 |
| glDraw              | `vkCmdBindVertexBuffers` + `vkCmdDraw` | 绑定缓冲区然后绘制 |

那么我们从顶层到底层，先定义一个简单的顶点数据：
```cpp
struct Vertexdata
{
    glm::vec3 Position;
    glm::vec3 Normal;
};
```
首先需要 **VkVertexInputBindingDescription**（绑定描述），这里具体参数目前其实不需要了解，默认参数即是在OpenGL中最常用的状态：
```cpp
VkVertexInputBindingDescription bindingDescription{};
bindingDescription.binding = 0; // 绑定编号
bindingDescription.stride = sizeof(Vertexdata); // 每个顶点步进 52字节
bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // 每个顶点一次
```
这里的binding和着色器里的binding修饰符没有任何关系，默认置0即可，inputRate是给实例化渲染用的，和OpenGL中调用drawInstance不同，vulkan通过在这里逐实例传入顶点数据，直接统一调用draw函数来绘制

然后是 **VkVertexInputAttributeDescription**（属性描述）
```cpp
std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

attributeDescriptions[0].binding = 0; // 都是绑定0
attributeDescriptions[0].location = 0; // 着色器 location 0
attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
attributeDescriptions[0].offset = offsetof(Vertexdata, Position);

attributeDescriptions[1].binding = 0;
attributeDescriptions[1].location = 1; // 着色器 location 1
attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
attributeDescriptions[1].offset = offsetof(Vertexdata, Normal);
```
对应着色器中：
```
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
```
然后是 **VkPipelineVertexInputStateCreateInfo**（整体顶点输入描述），把上面的 binding 和 attribute 描述放到 pipeline 里：
```cpp
VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
vertexInputInfo.vertexBindingDescriptionCount = 1;
vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
```
最后在绘制时绑定：
```cpp
vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
```
我们目前还没创建vertexBuffer，但已经可以看到，与OpenGL将VBO和VBOLayout统一放在VAO中不同，vulkan这边VBO和其Layout描述其实分开了

### Vertex Buffer
然后来创建一个VBO将数据真正传入显卡，先来一个基础版，步骤大概是：
1. 创建 VkBuffer
2. 获取显存需求
3. 分配显存
4. 绑定显存
5. 把数据复制进去

这里我们分配显存时，选择VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT，也就是cpu能直接访问和读写，对应代码比较多，就不写出来了，使用./App/VKBase.h中封装好的类的话大概是这样：
```cpp
std::vector<Vertexdata> vertices = {
    Vertexdata(1.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f),
    Vertexdata(-1.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f),
    Vertexdata(0.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f)
};

bufferMemory vertexBuffer;

// 1. 定义创建 VkBuffer 的参数
VkBufferCreateInfo bufferCreateInfo = {};
bufferCreateInfo.size = sizeof(Vertexdata) * vertices.size();
bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

// 2. 创建 buffer + 分配 memory + 绑定 memory
if (VkResult result = vertexBuffer.Create(bufferCreateInfo, 
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
    throw std::runtime_error("Failed to create CPU-writable vertex buffer!");
}

// 3. 上传顶点数据
if (VkResult result = vertexBuffer.BufferData(vertices.data(), bufferCreateInfo.size)) {
    throw std::runtime_error("Failed to upload vertex data to vertex buffer!");
}
```
### Staging Buffer
但像上面一样可以直接写入GPU显存，并不是最理想的状态，最理想的是把顶点数据放在 GPU 的「DeviceLocal」高速内存里，这样绘制速度最快。但「DeviceLocal」内存CPU无法直接写入，所以要先用「Staging Buffer」在CPU上准备数据，再复制（拷贝）到DeviceLocal的Buffer中。这里的Staging Buffer类似于上面一样创建的内存，其实就是多了一个将这块内存再复制到GPU高速显存的步骤

因此，现在的步骤变成了：
1. 创建一个 Staging Buffer

- `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | - VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`

- `VK_BUFFER_USAGE_TRANSFER_SRC_BIT`

2. 创建一个 DeviceLocal Buffer

- `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`

- `VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT`

3. 把数据拷贝到 Staging Buffer

- 用 `BufferData`

4. 用命令缓冲区 (vkCmdCopyBuffer)

- 把 Staging Buffer 拷贝到 DeviceLocal Buffer

5. 完成以后，Staging Buffer可以销毁

代码如下（仅供理解）：
```cpp
// 1. 创建staging buffer
bufferMemory stagingBuffer;

VkBufferCreateInfo stagingCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
stagingCreateInfo.size = sizeof(Vertexdata) * vertices.size();
stagingCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
stagingCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

if (VkResult result = stagingBuffer.Create(
    stagingCreateInfo,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
)) {
    throw std::runtime_error("Failed to create staging buffer!");
}
// 上传数据到 staging buffer
if (VkResult result = stagingBuffer.BufferData(vertices.data(), stagingCreateInfo.size)) {
    throw std::runtime_error("Failed to upload vertex data to staging buffer!");
}

// 2. 创建 DeviceLocal VertexBuffer
bufferMemory deviceLocalVertexBuffer;

VkBufferCreateInfo deviceBufferCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
deviceBufferCreateInfo.size = stagingCreateInfo.size;
deviceBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
deviceBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

if (VkResult result = deviceLocalVertexBuffer.Create(
    deviceBufferCreateInfo,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
)) {
    throw std::runtime_error("Failed to create device local vertex buffer!");
}

// 3. 用命令缓冲区拷贝数据
VkCommandBuffer commandBuffer = BeginSingleTimeCommands(); // 实际没这个函数，就是绑定单次命令缓冲区
VkBufferCopy copyRegion{};
copyRegion.srcOffset = 0;
copyRegion.dstOffset = 0;
copyRegion.size = stagingCreateInfo.size;
vkCmdCopyBuffer(commandBuffer, stagingBuffer.Buffer(), deviceLocalVertexBuffer.Buffer(), 1, &copyRegion);
EndSingleTimeCommands(commandBuffer); // 提交并等待完成
```








所以还是在./src/Mesh.hz中创建Mesh类，定义这样的顶点数据：
```cpp
struct Vertexdata
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;

    glm::vec3 Tangent;   // 切线
    float tangentW;      // 切线方向标志位（+1 或 -1）

    Vertexdata(float px = 0.0f, float py = 0.0f, float pz = 0.0f,
               float nx = 0.0f, float ny = 0.0f, float nz = 0.0f,
               float tx = 0.0f, float ty = 0.0f)
               : Position(px, py, pz), Normal(nx, ny, nz), TexCoords(tx, ty), 
               Tangent(0.0f), tangentW(1.0f) {}
};
```
基本上够用，大部分代码都是从**上个教程**里复制的，只需要修改其中的OpenGL接口为vulkan即可，这里我没有封装Vertex Buffer和Index Buffer类，因为后面主要实现路径追踪，顶点数据都会通过SSBO传入，这两个就用不上了，这里临时创建了Vertex Buffer和Index Buffer只是为了测试，具体创建过程中具体来说：
