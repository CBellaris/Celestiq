#pragma once
#include <vulkan/vulkan.h>
#include "vulkan/utility/vk_format_utils.h"
#include <format>
#include <iostream>
#include <cstring>
#include <span>
#include <fstream>

#include "Application.h" 
#include "stb_image.h"

#define DefineHandleTypeOperator operator decltype(handle)() const { return handle; } \
                                 auto getHandle() const {return handle; }
#define DefineAddressFunction const decltype(handle)* Address() const { return &handle; }

namespace Celestiq::Vulkan
{
    // 将单个对象转换为 span
    template<typename T>
    constexpr std::span<T> makeSpanFromOne(T& t) { return std::span<T>(&t, 1); }
    template<typename T>
    constexpr std::span<T> makeSpanFromOne(T* t) { return std::span<T>(t, 1); }

	inline uint32_t GetPixelOrBlockSize(VkFormat format) {
		const VKU_FORMAT_INFO info = vkuGetFormatInfo(format);
		// 如果每个block只包含一个像素，就直接返回像素大小
		return info.texel_block_size / info.texels_per_block;
	}		
	inline uint32_t GetComponentSize(VkFormat format) {
		const VKU_FORMAT_INFO info = vkuGetFormatInfo(format);
		return (info.texel_block_size / info.texels_per_block)/info.component_count;
	}	
	inline uint32_t GetComponentCount(VkFormat format) {
		const VKU_FORMAT_INFO info = vkuGetFormatInfo(format);
		return info.component_count;
	}

	inline VkResult SubmitCommandBuffer_Graphics(VkSubmitInfo &submitInfo, VkFence fence)
	{
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkResult result = vkQueueSubmit(Celestiq::Application::GetQueue(), 1, &submitInfo, fence);
		if (result)
			std::cout << std::format("[ graphicsBase ] ERROR\nFailed to submit the command buffer!\nError code: {}\n", int32_t(result));
		return result;
	}
	
	inline VkResult SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer, VkSemaphore semaphore_imageIsAvailable, VkSemaphore semaphore_renderingIsOver, VkFence fence, VkPipelineStageFlags waitDstStage_imageIsAvailable)
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
	
	inline VkResult SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer, VkFence fence)
	{
		VkSubmitInfo submitInfo = {
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer
		};
		return SubmitCommandBuffer_Graphics(submitInfo, fence);
	}

    class fence {
		VkFence handle = VK_NULL_HANDLE; // Vulkan 的 Fence 对象句柄
	public:
		// 构造函数，支持通过创建信息或标志创建 Fence
		fence(VkFenceCreateInfo& createInfo) {
			Create(createInfo);
		}
		fence(VkFenceCreateFlags flags = 0) {
			Create(flags);
		}
		// 移动构造函数
		fence(fence&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
		// 析构函数，销毁 Fence
		~fence() { if (handle) { vkDestroyFence(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Const Function
		// 等待 Fence 信号
		VkResult Wait() const {
			VkResult result = vkWaitForFences(Application::GetDevice(), 1, &handle, false, UINT64_MAX);
			if (result)
				std::cout << std::format("[ fence ] ERROR\nFailed to wait for the fence!\nError code: {}\n", int32_t(result));
			return result;
		}
		// 重置 Fence 状态
		VkResult Reset() const {
			VkResult result = vkResetFences(Application::GetDevice(), 1, &handle);
			if (result)
                std::cout << std::format("[ fence ] ERROR\nFailed to reset the fence!\nError code: {}\n", int32_t(result));
			return result;
		}
		// 等待并重置 Fence
		VkResult WaitAndReset() const {
			VkResult result = Wait();
			result || (result = Reset());
			return result;
		}
		// 获取 Fence 状态
		VkResult Status() const {
			VkResult result = vkGetFenceStatus(Application::GetDevice(), handle);
			if (result < 0)
                std::cout << std::format("[ fence ] ERROR\nFailed to get the status of the fence!\nError code: {}\n", int32_t(result));
			return result;
		}
		//Non-const Function
		// 创建 Fence
		VkResult Create(VkFenceCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            createInfo.pNext = nullptr;
			VkResult result = vkCreateFence(Application::GetDevice(), &createInfo, nullptr, &handle);
			if (result)
                std::cout << std::format("[ fence ] ERROR\nFailed to create a fence!\nError code: {}\n", int32_t(result));
			return result;
		}
		VkResult Create(VkFenceCreateFlags flags = 0) {
			VkFenceCreateInfo createInfo;
            createInfo.flags = flags;
			return Create(createInfo);
		}
	};

	inline VkResult ExecuteCommandBuffer_Graphics(VkCommandBuffer commandBuffer) {
		fence fence;
		VkSubmitInfo submitInfo = {
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer
		};
		VkResult result = SubmitCommandBuffer_Graphics(submitInfo, fence);
		if (!result)
			fence.Wait();
		return result;
	}

    class semaphore {
		VkSemaphore handle = VK_NULL_HANDLE; // Vulkan 的 Semaphore 对象句柄
	public:
		// 构造函数，支持通过创建信息创建 Semaphore
		semaphore(VkSemaphoreCreateInfo& createInfo) {
			Create(createInfo);
		}
		semaphore(/*reserved for future use*/) {
			Create();
		}
		semaphore(semaphore&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
		~semaphore() { if (handle) { vkDestroySemaphore(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Non-const Function
		// 创建 Semaphore
		VkResult Create(VkSemaphoreCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VkResult result = vkCreateSemaphore(Application::GetDevice(), &createInfo, nullptr, &handle);
			if (result)
                std::cout << std::format("[ semaphore ] ERROR\nFailed to create a semaphore!\nError code: {}\n", int32_t(result));
			return result;
		}
		VkResult Create(/*reserved for future use*/) {
			VkSemaphoreCreateInfo createInfo = {};
			return Create(createInfo);
		}
	};


    class commandBuffer {
		friend class commandPool; // 允许 commandPool 访问私有成员
		VkCommandBuffer handle = VK_NULL_HANDLE; // Vulkan 的 Command Buffer 对象句柄
	public:
		commandBuffer() = default;
		commandBuffer(commandBuffer&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Const Function
		// 开始记录命令
		VkResult Begin(VkCommandBufferUsageFlags usageFlags = 0) const {
			VkCommandBufferBeginInfo beginInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.flags = usageFlags,
			};
			VkResult result = vkBeginCommandBuffer(handle, &beginInfo);
			if (result)
                std::cout << std::format("[ commandBuffer ] ERROR\nFailed to begin a command buffer!\nError code: {}\n", int32_t(result));
			return result;
		}
		// 结束记录命令
		VkResult End() const {
			VkResult result = vkEndCommandBuffer(handle);
			if (result)
                std::cout << std::format("[ commandBuffer ] ERROR\nFailed to end a command buffer!\nError code: {}\n", int32_t(result));
			return result;
		}
	};


	class commandPool {
		VkCommandPool handle = VK_NULL_HANDLE; // Vulkan 的 Command Pool 对象句柄
	public:
		// 构造函数，支持通过创建信息或队列族索引创建 Command Pool
		commandPool() = default;
		commandPool(VkCommandPoolCreateInfo& createInfo) {
			Create(createInfo);
		}
		commandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
			Create(queueFamilyIndex, flags);
		}
		commandPool(commandPool&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
		~commandPool() { if (handle) { vkDestroyCommandPool(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Const Function
		// 分配 Command Buffers
		VkResult AllocateBuffers(std::span<VkCommandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
			VkCommandBufferAllocateInfo allocateInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = handle,
				.level = level,
				.commandBufferCount = uint32_t(buffers.size())
			};
			VkResult result = vkAllocateCommandBuffers(Application::GetDevice(), &allocateInfo, buffers.data());
			if (result)
                std::cout << std::format("[ commandPool ] ERROR\nFailed to allocate command buffers!\nError code: {}\n", int32_t(result));
			return result;
		}
		VkResult AllocateBuffers(std::span<commandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
			return AllocateBuffers(
				{ &buffers[0].handle, buffers.size() },
				level);
		}
		// 释放 Command Buffers
		void FreeBuffers(std::span<VkCommandBuffer> buffers) const {
			vkFreeCommandBuffers(Application::GetDevice(), handle, buffers.size(), buffers.data());
			memset(buffers.data(), 0, buffers.size() * sizeof(VkCommandBuffer));
		}
		void FreeBuffers(std::span<commandBuffer> buffers) const {
			FreeBuffers({ &buffers[0].handle, buffers.size() });
		}
		//Non-const Function
		// 创建 Command Pool
		VkResult Create(VkCommandPoolCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			VkResult result = vkCreateCommandPool(Application::GetDevice(), &createInfo, nullptr, &handle);
			if (result)
                std::cout << std::format("[ commandPool ] ERROR\nFailed to create a command pool!\nError code: {}\n", int32_t(result));
			return result;
		}
		VkResult Create(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
			VkCommandPoolCreateInfo createInfo = {
				.flags = flags,
				.queueFamilyIndex = queueFamilyIndex
			};
			return Create(createInfo);
		}
	};

    class renderPass {
		VkRenderPass handle = VK_NULL_HANDLE;
	public:
		renderPass() = default;
		renderPass(VkRenderPassCreateInfo& createInfo) {
			Create(createInfo);
		}
		renderPass(renderPass&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
		~renderPass() { if (handle) { vkDestroyRenderPass(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Const Function
		void CmdBegin(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo& beginInfo, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
			beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			beginInfo.renderPass = handle;
			vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
		}
		void CmdBegin(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkRect2D renderArea, std::span<const VkClearValue> clearValues = {}, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
			VkRenderPassBeginInfo beginInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.renderPass = handle,
				.framebuffer = framebuffer,
				.renderArea = renderArea,
				.clearValueCount = uint32_t(clearValues.size()),
				.pClearValues = clearValues.data()
			};
			vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
		}
		void CmdNext(VkCommandBuffer commandBuffer, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
			vkCmdNextSubpass(commandBuffer, subpassContents);
		}
		void CmdEnd(VkCommandBuffer commandBuffer) const {
			vkCmdEndRenderPass(commandBuffer);
		}
		//Non-const Function
		VkResult Create(VkRenderPassCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			VkResult result = vkCreateRenderPass(Application::GetDevice(), &createInfo, nullptr, &handle);
			if (result)
				std::cout << std::format("[ renderPass ] ERROR\nFailed to create a render pass!\nError code: {}\n", int32_t(result));
			return result;
		}
	};

    class framebuffer {
        VkFramebuffer handle = VK_NULL_HANDLE;
    public:
        framebuffer() = default;
        framebuffer(VkFramebufferCreateInfo& createInfo) {
            Create(createInfo);
        }
        framebuffer(framebuffer&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
        ~framebuffer() { if (handle) { vkDestroyFramebuffer(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; } }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Non-const Function
        VkResult Create(VkFramebufferCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            VkResult result = vkCreateFramebuffer(Application::GetDevice(), &createInfo, nullptr, &handle);
            if (result)
                std::cout << std::format("[ framebuffer ] ERROR\nFailed to create a framebuffer!\nError code: {}\n", int32_t(result));
            return result;
        }
    };

	class deviceMemory {
		VkDeviceMemory handle = VK_NULL_HANDLE;
		VkDeviceSize allocationSize = 0;
		VkMemoryPropertyFlags memoryProperties = 0;
		//--------------------
		VkDeviceSize AdjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const {
			const VkDeviceSize& nonCoherentAtomSize = Application::GetPhysicalDeviceProperties().limits.nonCoherentAtomSize;
			VkDeviceSize _offset = offset;
			offset = offset / nonCoherentAtomSize * nonCoherentAtomSize;
			size = std::min((size + _offset + nonCoherentAtomSize - 1) / nonCoherentAtomSize * nonCoherentAtomSize, allocationSize) - offset;
			return _offset - offset;
		}
	protected:
		class {
			friend class bufferMemory;
			friend class imageMemory;
			bool value = false;
			operator bool() const { return value; }
			auto& operator=(bool value) { this->value = value; return *this; }
		} areBound;
	public:
		deviceMemory() = default;
		deviceMemory(VkMemoryAllocateInfo& allocateInfo) {
			Allocate(allocateInfo);
		}
		deviceMemory(deviceMemory&& other) noexcept {
			handle = other.handle; other.handle = VK_NULL_HANDLE;
			allocationSize = other.allocationSize;
			memoryProperties = other.memoryProperties;
			other.allocationSize = 0;
			other.memoryProperties = 0;
		}
		~deviceMemory() { if (handle) { vkFreeMemory(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; } allocationSize = 0; memoryProperties = 0; }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		VkDeviceSize AllocationSize() const { return allocationSize; }
		VkMemoryPropertyFlags MemoryProperties() const { return memoryProperties; }
		//Const Function
		VkResult MapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset = 0) const {
			VkDeviceSize inverseDeltaOffset;
			if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
				inverseDeltaOffset = AdjustNonCoherentMemoryRange(size, offset);
			if (VkResult result = vkMapMemory(Application::GetDevice(), handle, offset, size, 0, &pData)) {
				std::cout << std::format("[ deviceMemory ] ERROR\nFailed to map the memory!\nError code: {}\n", int32_t(result));
				return result;
			}
			if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				pData = static_cast<uint8_t*>(pData) + inverseDeltaOffset;
				VkMappedMemoryRange mappedMemoryRange = {
					.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
					.memory = handle,
					.offset = offset,
					.size = size
				};
				if (VkResult result = vkInvalidateMappedMemoryRanges(Application::GetDevice(), 1, &mappedMemoryRange)) {
					std::cout << std::format("[ deviceMemory ] ERROR\nFailed to flush the memory!\nError code: {}\n", int32_t(result));
					return result;
				}
			}
			return VK_SUCCESS;
		}
		VkResult UnmapMemory(VkDeviceSize size, VkDeviceSize offset = 0) const {
			if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				AdjustNonCoherentMemoryRange(size, offset);
				VkMappedMemoryRange mappedMemoryRange = {
					.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
					.memory = handle,
					.offset = offset,
					.size = size
				};
				if (VkResult result = vkFlushMappedMemoryRanges(Application::GetDevice(), 1, &mappedMemoryRange)) {
					std::cout << std::format("[ deviceMemory ] ERROR\nFailed to flush the memory!\nError code: {}\n", int32_t(result));
					return result;
				}
			}
			vkUnmapMemory(Application::GetDevice(), handle);
			return VK_SUCCESS;
		}
		VkResult BufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const {
			void* pData_dst;
			if (VkResult result = MapMemory(pData_dst, size, offset))
				return result;
			memcpy(pData_dst, pData_src, size_t(size));
			return UnmapMemory(size, offset);
		}
		VkResult BufferData(const auto& data_src) const {
			return BufferData(&data_src, sizeof data_src);
		}
		VkResult RetrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset = 0) const {
			void* pData_src;
			if (VkResult result = MapMemory(pData_src, size, offset))
				return result;
			memcpy(pData_dst, pData_src, size_t(size));
			return UnmapMemory(size, offset);
		}
		//Non-const Function
		VkResult Allocate(VkMemoryAllocateInfo& allocateInfo) {
			if (allocateInfo.memoryTypeIndex >= Application::GetPhysicalDeviceMemoryProperties().memoryTypeCount) {
				std::cout << std::format("[ deviceMemory ] ERROR\nInvalid memory type index!\n");
				return VK_RESULT_MAX_ENUM;
			}
			allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			if (VkResult result = vkAllocateMemory(Application::GetDevice(), &allocateInfo, nullptr, &handle)) {
				std::cout << std::format("[ deviceMemory ] ERROR\nFailed to allocate memory!\nError code: {}\n", int32_t(result));
				return result;
			}
			allocationSize = allocateInfo.allocationSize;
			memoryProperties = Application::GetPhysicalDeviceMemoryProperties().memoryTypes[allocateInfo.memoryTypeIndex].propertyFlags;
			return VK_SUCCESS;
		}
	};

    class buffer {
		VkBuffer handle = VK_NULL_HANDLE;
	public:
		buffer() = default;
		buffer(VkBufferCreateInfo& createInfo) {
			Create(createInfo);
		}
		buffer(buffer&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
		~buffer() { if (handle) { vkDestroyBuffer(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; }}
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Const Function
		VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const {
			VkMemoryAllocateInfo memoryAllocateInfo = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
			};
			VkMemoryRequirements memoryRequirements;
			vkGetBufferMemoryRequirements(Application::GetDevice(), handle, &memoryRequirements);
			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			memoryAllocateInfo.memoryTypeIndex = UINT32_MAX;
			auto& physicalDeviceMemoryProperties = Application::GetPhysicalDeviceMemoryProperties();
			for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
				if (memoryRequirements.memoryTypeBits & 1 << i &&
					(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties) {
					memoryAllocateInfo.memoryTypeIndex = i;
					break;
				}
			return memoryAllocateInfo;
		}
		VkResult BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const {
			VkResult result = vkBindBufferMemory(Application::GetDevice(), handle, deviceMemory, memoryOffset);
			if (result)
				std::cout << std::format("[ buffer ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
			return result;
		}
		//Non-const Function
		VkResult Create(VkBufferCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			VkResult result = vkCreateBuffer(Application::GetDevice(), &createInfo, nullptr, &handle);
			if (result)
				std::cout << std::format("[ buffer ] ERROR\nFailed to create a buffer!\nError code: {}\n", int32_t(result));
			return result;
		}
	};

	class bufferMemory :buffer, deviceMemory {
	public:
		bufferMemory() = default;
		bufferMemory(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
			Create(createInfo, desiredMemoryProperties);
		}
		bufferMemory(bufferMemory&& other) noexcept :
			buffer(std::move(other)), deviceMemory(std::move(other)) {
			areBound = other.areBound;
			other.areBound = false;
		}
		~bufferMemory() { areBound = false; }
		//Getter
		VkBuffer Buffer() const { return static_cast<const buffer&>(*this); }
		const VkBuffer* AddressOfBuffer() const { return buffer::Address(); }
		VkDeviceMemory Memory() const { return static_cast<const deviceMemory&>(*this); }
		const VkDeviceMemory* AddressOfMemory() const { return deviceMemory::Address(); }
		bool AreBound() const { return areBound; }
		using deviceMemory::AllocationSize;
		using deviceMemory::MemoryProperties;
		//Const Function
		using deviceMemory::MapMemory;
		using deviceMemory::UnmapMemory;
		using deviceMemory::BufferData;
		using deviceMemory::RetrieveData;
		//Non-const Function
		VkResult CreateBuffer(VkBufferCreateInfo& createInfo) {
			return buffer::Create(createInfo);
		}
		VkResult AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties) {
			VkMemoryAllocateInfo allocateInfo = MemoryAllocateInfo(desiredMemoryProperties);
			if (allocateInfo.memoryTypeIndex >= Application::GetPhysicalDeviceMemoryProperties().memoryTypeCount)
				return VK_RESULT_MAX_ENUM;
			return Allocate(allocateInfo);
		}
		VkResult BindMemory() {
			if (VkResult result = buffer::BindMemory(Memory()))
				return result;
			areBound = true;
			return VK_SUCCESS;
		}
		VkResult Create(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
			VkResult result;
			false ||
				(result = CreateBuffer(createInfo)) ||
				(result = AllocateMemory(desiredMemoryProperties)) ||
				(result = BindMemory());
			return result;
		}
	};


    class image {
		VkImage handle = VK_NULL_HANDLE;
	public:
		image() = default;
		image(VkImageCreateInfo& createInfo) {
			Create(createInfo);
		}
		image(image&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
		~image() { if (handle) { vkDestroyImage(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Const Function
		VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const {
			VkMemoryAllocateInfo memoryAllocateInfo = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
			};
			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(Application::GetDevice(), handle, &memoryRequirements);
			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			auto GetMemoryTypeIndex = [](uint32_t memoryTypeBits, VkMemoryPropertyFlags desiredMemoryProperties) {
				auto& physicalDeviceMemoryProperties = Application::GetPhysicalDeviceMemoryProperties();
				for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
					if (memoryTypeBits & 1 << i &&
						(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties)
						return i;
				return UINT32_MAX;
			};
			memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties);
			if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX &&
				desiredMemoryProperties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
				memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties & ~VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
			return memoryAllocateInfo;
		}
		VkResult BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const {
			VkResult result = vkBindImageMemory(Application::GetDevice(), handle, deviceMemory, memoryOffset);
			if (result)
				std::cout << std::format("[ image ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
			return result;
		}
		//Non-const Function
		VkResult Create(VkImageCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			VkResult result = vkCreateImage(Application::GetDevice(), &createInfo, nullptr, &handle);
			if (result)
                std::cout << std::format("[ image ] ERROR\nFailed to create an image!\nError code: {}\n", int32_t(result));
			return result;
		}
	};


    class imageMemory :image, deviceMemory {
    public:
        imageMemory() = default;
        imageMemory(VkImageCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
            Create(createInfo, desiredMemoryProperties);
        }
        imageMemory(imageMemory&& other) noexcept :
            image(std::move(other)), deviceMemory(std::move(other)) {
            areBound = other.areBound;
            other.areBound = false;
        }
        ~imageMemory() { areBound = false; }
        //Getter
        VkImage Image() const { return static_cast<const image&>(*this); }
        const VkImage* AddressOfImage() const { return image::Address(); }
        VkDeviceMemory Memory() const { return static_cast<const deviceMemory&>(*this); }
        const VkDeviceMemory* AddressOfMemory() const { return deviceMemory::Address(); }
        bool AreBound() const { return areBound; }
        using deviceMemory::AllocationSize;
        using deviceMemory::MemoryProperties;
        //Non-const Function
        VkResult CreateImage(VkImageCreateInfo& createInfo) {
            return image::Create(createInfo);
        }
        VkResult AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties) {
            VkMemoryAllocateInfo allocateInfo = MemoryAllocateInfo(desiredMemoryProperties);
            if (allocateInfo.memoryTypeIndex >= Application::GetPhysicalDeviceMemoryProperties().memoryTypeCount)
                return VK_RESULT_MAX_ENUM;
            return Allocate(allocateInfo);
        }
        VkResult BindMemory() {
            if (VkResult result = image::BindMemory(Memory()))
                return result;
            areBound = true;
            return VK_SUCCESS;
        }
        VkResult Create(VkImageCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties) {
            VkResult result;
            false ||
                (result = CreateImage(createInfo)) ||
                (result = AllocateMemory(desiredMemoryProperties)) ||
                (result = BindMemory());
            return result;
        }
    };


    class imageView {
        VkImageView handle = VK_NULL_HANDLE;
    public:
        imageView() = default;
        imageView(VkImageViewCreateInfo& createInfo) {
            Create(createInfo);
        }
        imageView(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0) {
            Create(image, viewType, format, subresourceRange, flags);
        }
        imageView(imageView&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
        ~imageView() { if (handle) { vkDestroyImageView(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; } }
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        //Non-const Function
        VkResult Create(VkImageViewCreateInfo& createInfo) {
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            VkResult result = vkCreateImageView(Application::GetDevice(), &createInfo, nullptr, &handle);
            if (result)
                std::cout << std::format("[ imageView ] ERROR\nFailed to create an image view!\nError code: {}\n", int32_t(result));
            return result;
        }
        VkResult Create(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0) {
            VkImageViewCreateInfo createInfo = {
                .flags = flags,
                .image = image,
                .viewType = viewType,
                .format = format,
                .subresourceRange = subresourceRange
            };
            return Create(createInfo);
        }
    };

    class sampler {
		VkSampler handle = VK_NULL_HANDLE;
	public:
		sampler() = default;
		sampler(VkSamplerCreateInfo& createInfo) {
			Create(createInfo);
		}
		sampler(sampler&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
		~sampler() { if (handle) { vkDestroySampler(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Non-const Function
		VkResult Create(VkSamplerCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			VkResult result = vkCreateSampler(Application::GetDevice(), &createInfo, nullptr, &handle);
			if (result)
				std::cout << std::format("[ sampler ] ERROR\nFailed to create a sampler!\nError code: {}\n", int32_t(result));
			return result;
		}
	};

    class descriptorSetLayout {
		VkDescriptorSetLayout handle = VK_NULL_HANDLE;
	public:
		descriptorSetLayout() = default;
		descriptorSetLayout(VkDescriptorSetLayoutCreateInfo& createInfo) {
			Create(createInfo);
		}
		descriptorSetLayout(descriptorSetLayout&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
		~descriptorSetLayout() { if (handle) { vkDestroyDescriptorSetLayout(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Non-const Function
		VkResult Create(VkDescriptorSetLayoutCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			VkResult result = vkCreateDescriptorSetLayout(Application::GetDevice(), &createInfo, nullptr, &handle);
			if (result)
				std::cout << std::format("[ descriptorSetLayout ] ERROR\nFailed to create a descriptor set layout!\nError code: {}\n", int32_t(result));
			return result;
		}
	};

    class descriptorSet {
		friend class descriptorPool;
		VkDescriptorSet handle = VK_NULL_HANDLE;
	public:
		descriptorSet() = default;
		descriptorSet(descriptorSet&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Const Function
		void Write(std::span<const VkDescriptorImageInfo> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const {
			VkWriteDescriptorSet writeDescriptorSet = {
				.dstSet = handle,
				.dstBinding = dstBinding,
				.dstArrayElement = dstArrayElement,
				.descriptorCount = uint32_t(descriptorInfos.size()),
				.descriptorType = descriptorType,
				.pImageInfo = descriptorInfos.data()
			};
			Update(makeSpanFromOne(writeDescriptorSet));
		}
		void Write(std::span<const VkDescriptorBufferInfo> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const {
			VkWriteDescriptorSet writeDescriptorSet = {
				.dstSet = handle,
				.dstBinding = dstBinding,
				.dstArrayElement = dstArrayElement,
				.descriptorCount = uint32_t(descriptorInfos.size()),
				.descriptorType = descriptorType,
				.pBufferInfo = descriptorInfos.data()
			};
			Update(makeSpanFromOne(writeDescriptorSet));
		}
		void Write(std::span<const VkBufferView> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const {
			VkWriteDescriptorSet writeDescriptorSet = {
				.dstSet = handle,
				.dstBinding = dstBinding,
				.dstArrayElement = dstArrayElement,
				.descriptorCount = uint32_t(descriptorInfos.size()),
				.descriptorType = descriptorType,
				.pTexelBufferView = descriptorInfos.data()
			};
			Update(makeSpanFromOne(writeDescriptorSet));
		}

		//Static Function
		static void Update(std::span<VkWriteDescriptorSet> writes, std::span<VkCopyDescriptorSet> copies = {}) {
			for (auto& i : writes)
				i.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			for (auto& i : copies)
				i.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			vkUpdateDescriptorSets(
				Application::GetDevice(), writes.size(), writes.data(), copies.size(), copies.data());
		}
	};

    class descriptorPool {
		VkDescriptorPool handle = VK_NULL_HANDLE;
	public:
		descriptorPool() = default;
		descriptorPool(VkDescriptorPoolCreateInfo& createInfo) {
			Create(createInfo);
		}
		descriptorPool(uint32_t maxSetCount, std::span<const VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags flags = 0) {
			Create(maxSetCount, poolSizes, flags);
		}
		descriptorPool(descriptorPool&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
		~descriptorPool() { if (handle) { vkDestroyDescriptorPool(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Const Function
		VkResult AllocateSets(std::span<VkDescriptorSet> sets, std::span<const VkDescriptorSetLayout> setLayouts) const {
			if (sets.size() != setLayouts.size())
				if (sets.size() < setLayouts.size()) {
					std::cout << std::format("[ descriptorPool ] ERROR\nFor each descriptor set, must provide a corresponding layout!\n");
					return VK_RESULT_MAX_ENUM;
				}
				else
                    std::cout << std::format("[ descriptorPool ] WARNING\nProvided layouts are more than sets!\n");
			VkDescriptorSetAllocateInfo allocateInfo = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = handle,
				.descriptorSetCount = uint32_t(sets.size()),
				.pSetLayouts = setLayouts.data()
			};
			VkResult result = vkAllocateDescriptorSets(Application::GetDevice(), &allocateInfo, sets.data());
			if (result)
                std::cout << std::format("[ descriptorPool ] ERROR\nFailed to allocate descriptor sets!\nError code: {}\n", int32_t(result));
			return result;
		}
		VkResult AllocateSets(std::span<VkDescriptorSet> sets, std::span<const descriptorSetLayout> setLayouts) const {
			return AllocateSets(
				sets,
				{ setLayouts[0].Address(), setLayouts.size() });
		}
		VkResult AllocateSets(std::span<descriptorSet> sets, std::span<const VkDescriptorSetLayout> setLayouts) const {
			return AllocateSets(
				{ &sets[0].handle, sets.size() },
				setLayouts);
		}
		VkResult AllocateSets(std::span<descriptorSet> sets, std::span<const descriptorSetLayout> setLayouts) const {
			return AllocateSets(
				{ &sets[0].handle, sets.size() },
				{ setLayouts[0].Address(), setLayouts.size() });
		}
		VkResult FreeSets(std::span<VkDescriptorSet> sets) const {
			VkResult result = vkFreeDescriptorSets(Application::GetDevice(), handle, sets.size(), sets.data());
			memset(sets.data(), 0, sets.size() * sizeof(VkDescriptorSet));
			return result;//Though vkFreeDescriptorSets(...) can only return VK_SUCCESS
		}
		VkResult FreeSets(std::span<descriptorSet> sets) const {
			return FreeSets({ &sets[0].handle, sets.size() });
		}
		//Non-const Function
		VkResult Create(VkDescriptorPoolCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			VkResult result = vkCreateDescriptorPool(Application::GetDevice(), &createInfo, nullptr, &handle);
			if (result)
				std::cout << std::format("[ descriptorPool ] ERROR\nFailed to create a descriptor pool!\nError code: {}\n", int32_t(result));
			return result;
		}
		VkResult Create(uint32_t maxSetCount, std::span<const VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags flags = 0) {
			VkDescriptorPoolCreateInfo createInfo = {
				.flags = flags,
				.maxSets = maxSetCount,
				.poolSizeCount = uint32_t(poolSizes.size()),
				.pPoolSizes = poolSizes.data()
			};
			return Create(createInfo);
		}
	};

    class shaderModule {
		VkShaderModule handle = VK_NULL_HANDLE;
	public:
		shaderModule() = default;
		shaderModule(VkShaderModuleCreateInfo& createInfo) {
			Create(createInfo);
		}
		shaderModule(const char* filepath /*reserved for future use*/) {
			Create(filepath);
		}
		shaderModule(size_t codeSize, const uint32_t* pCode /*reserved for future use*/) {
			Create(codeSize, pCode);
		}
		shaderModule(shaderModule&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
		~shaderModule() { if (handle) { vkDestroyShaderModule(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Const Function
		VkPipelineShaderStageCreateInfo StageCreateInfo(VkShaderStageFlagBits stage, const char* entry = "main") const {
			return {
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,//sType
				nullptr,                                            //pNext
				0,                                                  //flags
				stage,                                              //stage
				handle,                                             //module
				entry,                                              //pName
				nullptr                                             //pSpecializationInfo
			};
		}
		//Non-const Function
		VkResult Create(VkShaderModuleCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			VkResult result = vkCreateShaderModule(Application::GetDevice(), &createInfo, nullptr, &handle);
			if (result)
				std::cout << std::format("[ shader ] ERROR\nFailed to create a shader module!\nError code: {}\n", int32_t(result));
			return result;
		}
		VkResult Create(const char* filepath /*reserved for future use*/) {
			std::ifstream file(filepath, std::ios::ate | std::ios::binary);
			if (!file) {
				std::cout << std::format("[ shader ] ERROR\nFailed to open the file: {}\n", filepath);
				return VK_RESULT_MAX_ENUM;//No proper VkResult enum value, don't use VK_ERROR_UNKNOWN
			}
			size_t fileSize = size_t(file.tellg());
			std::vector<uint32_t> binaries(fileSize / 4);
			file.seekg(0);
			file.read(reinterpret_cast<char*>(binaries.data()), fileSize);
			file.close();
			return Create(fileSize, binaries.data());
		}
		VkResult Create(size_t codeSize, const uint32_t* pCode /*reserved for future use*/) {
			VkShaderModuleCreateInfo createInfo = {
				.codeSize = codeSize,
				.pCode = pCode
			};
			return Create(createInfo);
		}
	};

    class pipelineLayout {
		VkPipelineLayout handle = VK_NULL_HANDLE;
	public:
		pipelineLayout() = default;
		pipelineLayout(VkPipelineLayoutCreateInfo& createInfo) {
			Create(createInfo);
		}
		pipelineLayout(pipelineLayout&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
		~pipelineLayout() { if (handle) { vkDestroyPipelineLayout(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; }}
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Non-const Function
		VkResult Create(VkPipelineLayoutCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			VkResult result = vkCreatePipelineLayout(Application::GetDevice(), &createInfo, nullptr, &handle);
			if (result)
				std::cout << std::format("[ pipelineLayout ] ERROR\nFailed to create a pipeline layout!\nError code: {}\n", int32_t(result));
			return result;
		}
	};

	class pipeline {
		VkPipeline handle = VK_NULL_HANDLE;
	public:
		pipeline() = default;
		pipeline(VkGraphicsPipelineCreateInfo& createInfo) {
			Create(createInfo);
		}
		pipeline(VkComputePipelineCreateInfo& createInfo) {
			Create(createInfo);
		}
		pipeline(pipeline&& other) noexcept { handle = other.handle; other.handle = VK_NULL_HANDLE; }
		~pipeline() { if (handle) { vkDestroyPipeline(Application::GetDevice(), handle, nullptr); handle = VK_NULL_HANDLE; } }
		//Getter
		DefineHandleTypeOperator;
		DefineAddressFunction;
		//Non-const Function
		VkResult Create(VkGraphicsPipelineCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			VkResult result = vkCreateGraphicsPipelines(Application::GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
			if (result)
				std::cout << std::format("[ pipeline ] ERROR\nFailed to create a graphics pipeline!\nError code: {}\n", int32_t(result));
			return result;
		}
		VkResult Create(VkComputePipelineCreateInfo& createInfo) {
			createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			VkResult result = vkCreateComputePipelines(Application::GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
			if (result)
                std::cout << std::format("[ pipeline ] ERROR\nFailed to create a compute pipeline!\nError code: {}\n", int32_t(result));
			return result;
		}
	};

    struct graphicsPipelineCreateInfoPack {
		VkGraphicsPipelineCreateInfo createInfo =
		{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		//Vertex Input
		VkPipelineVertexInputStateCreateInfo vertexInputStateCi =
		{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		std::vector<VkVertexInputBindingDescription> vertexInputBindings;
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
		//Input Assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCi =
		{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		//Tessellation
		VkPipelineTessellationStateCreateInfo tessellationStateCi =
		{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
		//Viewport
		VkPipelineViewportStateCreateInfo viewportStateCi =
		{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		std::vector<VkViewport> viewports;
		std::vector<VkRect2D> scissors;
		uint32_t dynamicViewportCount = 1;
		uint32_t dynamicScissorCount = 1;
		//Rasterization
		VkPipelineRasterizationStateCreateInfo rasterizationStateCi =
		{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		//Multisample
		VkPipelineMultisampleStateCreateInfo multisampleStateCi =
		{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		//Depth & Stencil
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCi =
		{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		//Color Blend
		VkPipelineColorBlendStateCreateInfo colorBlendStateCi =
		{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
		//Dynamic
		VkPipelineDynamicStateCreateInfo dynamicStateCi =
		{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		std::vector<VkDynamicState> dynamicStates;
		//--------------------
		graphicsPipelineCreateInfoPack() {
			SetCreateInfos();
			createInfo.basePipelineIndex = -1;
		}
		graphicsPipelineCreateInfoPack(const graphicsPipelineCreateInfoPack& other) noexcept {
			createInfo = other.createInfo;
			SetCreateInfos();

			vertexInputStateCi = other.vertexInputStateCi;
			inputAssemblyStateCi = other.inputAssemblyStateCi;
			tessellationStateCi = other.tessellationStateCi;
			viewportStateCi = other.viewportStateCi;
			rasterizationStateCi = other.rasterizationStateCi;
			multisampleStateCi = other.multisampleStateCi;
			depthStencilStateCi = other.depthStencilStateCi;
			colorBlendStateCi = other.colorBlendStateCi;
			dynamicStateCi = other.dynamicStateCi;

			shaderStages = other.shaderStages;
			vertexInputBindings = other.vertexInputBindings;
			vertexInputAttributes = other.vertexInputAttributes;
			viewports = other.viewports;
			scissors = other.scissors;
			colorBlendAttachmentStates = other.colorBlendAttachmentStates;
			dynamicStates = other.dynamicStates;
			UpdateAllArrayAddresses();
		}
		//Getter
		operator VkGraphicsPipelineCreateInfo& () { return createInfo; }
		//Non-const Function
		void UpdateAllArrays() {
			createInfo.stageCount = shaderStages.size();
			vertexInputStateCi.vertexBindingDescriptionCount = vertexInputBindings.size();
			vertexInputStateCi.vertexAttributeDescriptionCount = vertexInputAttributes.size();
			viewportStateCi.viewportCount = viewports.size() ? uint32_t(viewports.size()) : dynamicViewportCount;
			viewportStateCi.scissorCount = scissors.size() ? uint32_t(scissors.size()) : dynamicScissorCount;
			colorBlendStateCi.attachmentCount = colorBlendAttachmentStates.size();
			dynamicStateCi.dynamicStateCount = dynamicStates.size();
			UpdateAllArrayAddresses();
		}
	private:
		void SetCreateInfos() {
			createInfo.pVertexInputState = &vertexInputStateCi;
			createInfo.pInputAssemblyState = &inputAssemblyStateCi;
			createInfo.pTessellationState = &tessellationStateCi;
			createInfo.pViewportState = &viewportStateCi;
			createInfo.pRasterizationState = &rasterizationStateCi;
			createInfo.pMultisampleState = &multisampleStateCi;
			createInfo.pDepthStencilState = &depthStencilStateCi;
			createInfo.pColorBlendState = &colorBlendStateCi;
			createInfo.pDynamicState = &dynamicStateCi;
		}
		void UpdateAllArrayAddresses() {
			createInfo.pStages = shaderStages.data();
			vertexInputStateCi.pVertexBindingDescriptions = vertexInputBindings.data();
			vertexInputStateCi.pVertexAttributeDescriptions = vertexInputAttributes.data();
			viewportStateCi.pViewports = viewports.data();
			viewportStateCi.pScissors = scissors.data();
			colorBlendStateCi.pAttachments = colorBlendAttachmentStates.data();
			dynamicStateCi.pDynamicStates = dynamicStates.data();
		}
	};

	class stagingBuffer {
		static stagingBuffer stagingBuffer_mainThread;
	protected:
		bufferMemory bufferMemory;
		VkDeviceSize memoryUsage = 0;
		image aliasedImage;
	public:
		stagingBuffer() = default;
		stagingBuffer(VkDeviceSize size) {
			Expand(size);
		}
		//Getter
		operator VkBuffer() const { return bufferMemory.Buffer(); }
		const VkBuffer* Address() const { return bufferMemory.AddressOfBuffer(); }
		VkDeviceSize AllocationSize() const { return bufferMemory.AllocationSize(); }
		VkImage AliasedImage() const { return aliasedImage; }
		//Const Function
		void RetrieveData(void* pData_src, VkDeviceSize size) const {
			bufferMemory.RetrieveData(pData_src, size);
		}
		//Non-const Function
		void Expand(VkDeviceSize size) {
			if (size <= AllocationSize())
				return;
			Release();
			VkBufferCreateInfo bufferCreateInfo = {
				.size = size,
				.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			};
			bufferMemory.Create(bufferCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		}
		void Release() {
			bufferMemory.~bufferMemory();
		}
		void* MapMemory(VkDeviceSize size) {
			Expand(size);
			void* pData_dst = nullptr;
			bufferMemory.MapMemory(pData_dst, size);
			memoryUsage = size;
			return pData_dst;
		}
		void UnmapMemory() {
			bufferMemory.UnmapMemory(memoryUsage);
			memoryUsage = 0;
		}
		void BufferData(const void* pData_src, VkDeviceSize size) {
			Expand(size);
			bufferMemory.BufferData(pData_src, size);
		}

		[[nodiscard]]
		VkImage AliasedImage2d(VkFormat format, VkExtent2D extent) {
			// if (!(FormatProperties(format).linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
			// 	return VK_NULL_HANDLE;
			VkDeviceSize imageDataSize = VkDeviceSize(GetPixelOrBlockSize(format)) * extent.width * extent.height;
			if (imageDataSize > AllocationSize())
				return VK_NULL_HANDLE;
			VkImageFormatProperties imageFormatProperties = {};
			vkGetPhysicalDeviceImageFormatProperties(Application::GetPhysicalDevice(),
				format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0, &imageFormatProperties);
			if (extent.width > imageFormatProperties.maxExtent.width ||
				extent.height > imageFormatProperties.maxExtent.height ||
				imageDataSize > imageFormatProperties.maxResourceSize)
				return VK_NULL_HANDLE;
			VkImageCreateInfo imageCreateInfo = {
				.imageType = VK_IMAGE_TYPE_2D,
				.format = format,
				.extent = { extent.width, extent.height, 1 },
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_LINEAR,
				.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED
			};
			aliasedImage.~image();
			aliasedImage.Create(imageCreateInfo);
			VkImageSubresource subResource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
			VkSubresourceLayout subresourceLayout = {};
			vkGetImageSubresourceLayout(Application::GetDevice(), aliasedImage, &subResource, &subresourceLayout);
			if (subresourceLayout.size != imageDataSize)
				return VK_NULL_HANDLE;//No padding bytes
			aliasedImage.BindMemory(bufferMemory.Memory());
			return aliasedImage;
		}
		
		//Static Function
		static VkBuffer Buffer_MainThread() {
			return stagingBuffer_mainThread;
		}
		static void Expand_MainThread(VkDeviceSize size) {
			stagingBuffer_mainThread.Expand(size);
		}
		static void Release_MainThread() {
			stagingBuffer_mainThread.Release();
		}
		static void* MapMemory_MainThread(VkDeviceSize size) {
			return stagingBuffer_mainThread.MapMemory(size);
		}
		static void UnmapMemory_MainThread() {
			stagingBuffer_mainThread.UnmapMemory();
		}
		static void BufferData_MainThread(const void* pData_src, VkDeviceSize size) {
			stagingBuffer_mainThread.BufferData(pData_src, size);
		}
		static void RetrieveData_MainThread(void* pData_src, VkDeviceSize size) {
			stagingBuffer_mainThread.RetrieveData(pData_src, size);
		}
		[[nodiscard]]
		static VkImage AliasedImage2d_MainThread(VkFormat format, VkExtent2D extent) {
			return stagingBuffer_mainThread.AliasedImage2d(format, extent);
		}
	};
	inline stagingBuffer stagingBuffer::stagingBuffer_mainThread;

	class deviceLocalBuffer {
	protected:
		bufferMemory bufferMemory;
	public:
		deviceLocalBuffer() = default;
		deviceLocalBuffer(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst) {
			Create(size, desiredUsages_Without_transfer_dst);
		}
		//Getter
		operator VkBuffer() const { return bufferMemory.Buffer(); }
		VkBuffer getHandle() { return bufferMemory.Buffer(); }
		const VkBuffer* Address() const { return bufferMemory.AddressOfBuffer(); }
		VkDeviceSize AllocationSize() const { return bufferMemory.AllocationSize(); }
		//Const Function
		void TransferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const {
			if (bufferMemory.MemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
				bufferMemory.BufferData(pData_src, size, offset);
				return;
			}
			stagingBuffer::BufferData_MainThread(pData_src, size);

			std::shared_ptr<commandBuffer> commandBuffer = Celestiq::Application::Get().GetCommandBufferTransfer();
			commandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			VkBufferCopy region = { 0, offset, size };
			vkCmdCopyBuffer(commandBuffer->getHandle(), stagingBuffer::Buffer_MainThread(), bufferMemory.Buffer(), 1, &region);
			commandBuffer->End();

			fence fence;
			VkSubmitInfo submitInfo = {
				.commandBufferCount = 1,
				.pCommandBuffers = commandBuffer->Address()
			};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			VkResult result = vkQueueSubmit(Celestiq::Application::GetQueue(), 1, &submitInfo, fence);
			if (result)
				std::cout << std::format("[ deviceLocalBuffer ] ERROR\nFailed to submit the command buffer!\nError code: {}\n", int32_t(result));
			if (!result)
				fence.Wait();
		}
		void TransferData(const void* pData_src, uint32_t elementCount, VkDeviceSize elementSize, VkDeviceSize stride_src, VkDeviceSize stride_dst, VkDeviceSize offset = 0) const {
			if (bufferMemory.MemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
				void* pData_dst = nullptr;
				bufferMemory.MapMemory(pData_dst, stride_dst * elementCount, offset);
				for (size_t i = 0; i < elementCount; i++)
					memcpy(stride_dst * i + static_cast<uint8_t*>(pData_dst), stride_src * i + static_cast<const uint8_t*>(pData_src), size_t(elementSize));
				bufferMemory.UnmapMemory(elementCount * stride_dst, offset);
				return;
			}
			stagingBuffer::BufferData_MainThread(pData_src, stride_src * elementCount);
			std::shared_ptr<commandBuffer> commandBuffer = Celestiq::Application::Get().GetCommandBufferTransfer();
			commandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			std::unique_ptr<VkBufferCopy[]> regions = std::make_unique<VkBufferCopy[]>(elementCount);
			for (size_t i = 0; i < elementCount; i++)
				regions[i] = { stride_src * i, stride_dst * i + offset, elementSize };
			vkCmdCopyBuffer(commandBuffer->getHandle(), stagingBuffer::Buffer_MainThread(), bufferMemory.Buffer(), elementCount, regions.get());
			commandBuffer->End();
				
			fence fence;
			VkSubmitInfo submitInfo = {
				.commandBufferCount = 1,
				.pCommandBuffers = commandBuffer->Address()
			};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			VkResult result = vkQueueSubmit(Celestiq::Application::GetQueue(), 1, &submitInfo, fence);
			if (result)
				std::cout << std::format("[ deviceLocalBuffer ] ERROR\nFailed to submit the command buffer!\nError code: {}\n", int32_t(result));
			if (!result)
				fence.Wait();
		}
		void TransferData(const auto& data_src) const {
			TransferData(&data_src, sizeof data_src);
		}
		void CmdUpdateBuffer(VkCommandBuffer commandBuffer, const void* pData_src, VkDeviceSize size_Limited_to_65536, VkDeviceSize offset = 0) const {
			vkCmdUpdateBuffer(commandBuffer, bufferMemory.Buffer(), offset, size_Limited_to_65536, pData_src);
		}
		void CmdUpdateBuffer(VkCommandBuffer commandBuffer, const auto& data_src) const {
			vkCmdUpdateBuffer(commandBuffer, bufferMemory.Buffer(), 0, sizeof data_src, &data_src);
		}
		//Non-const Function
		void Create(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst) {
			VkBufferCreateInfo bufferCreateInfo = {
				.size = size,
				.usage = desiredUsages_Without_transfer_dst | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			};
			false ||
				bufferMemory.CreateBuffer(bufferCreateInfo) ||
				bufferMemory.AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
				bufferMemory.AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ||
				bufferMemory.BindMemory();
		}
		void Recreate(VkDeviceSize size, VkBufferUsageFlags desiredUsages_Without_transfer_dst) {
			VkResult result = vkDeviceWaitIdle(Application::GetDevice());
			if (result)
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to wait for the device to be idle!\nError code: {}\n", int32_t(result));
			bufferMemory.~bufferMemory();
			Create(size, desiredUsages_Without_transfer_dst);
		}
	};

	class vertexBuffer :public deviceLocalBuffer {
	public:
		vertexBuffer() = default;
		vertexBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) :deviceLocalBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages) {}
		//Non-const Function
		void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Create(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages);
		}
		void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages);
		}
	};
	class indexBuffer :public deviceLocalBuffer {
	public:
		indexBuffer() = default;
		indexBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) :deviceLocalBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsages) {}
		//Non-const Function
		void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Create(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsages);
		}
		void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsages);
		}
	};
	class uniformBuffer :public deviceLocalBuffer {
	public:
		uniformBuffer() = default;
		uniformBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) :deviceLocalBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsages) {}
		//Non-const Function
		void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Create(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsages);
		}
		void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsages);
		}
		//Static Function
		static VkDeviceSize CalculateAlignedSize(VkDeviceSize dataSize) {
			const VkDeviceSize& alignment = Application::GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
			return alignment + dataSize - 1 & ~(alignment - 1);
		}
	};
	class storageBuffer :public deviceLocalBuffer {
	public:
		storageBuffer() = default;
		storageBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) :deviceLocalBuffer(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsages) {}
		//Non-const Function
		void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Create(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsages);
		}
		void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsages);
		}
		//Static Function
		static VkDeviceSize CalculateAlignedSize(VkDeviceSize dataSize) {
			const VkDeviceSize& alignment = Application::GetPhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;
			return alignment + dataSize - 1 & ~(alignment - 1);
		}
	};

	struct imageOperation {
		struct imageMemoryBarrierParameterPack {
			const bool isNeeded = false;
			const VkPipelineStageFlags stage = 0;
			const VkAccessFlags access = 0;
			const VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
			constexpr imageMemoryBarrierParameterPack() = default;
			constexpr imageMemoryBarrierParameterPack(VkPipelineStageFlags stage, VkAccessFlags access, VkImageLayout layout) :
				isNeeded(true), stage(stage), access(access), layout(layout) {}
		};
		static void CmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, const VkBufferImageCopy& region,
			imageMemoryBarrierParameterPack imb_from, imageMemoryBarrierParameterPack imb_to) {
			//Pre-copy barrier
			VkImageMemoryBarrier imageMemoryBarrier = {
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				imb_from.access,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				imb_from.layout,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_QUEUE_FAMILY_IGNORED,//No ownership transfer
				VK_QUEUE_FAMILY_IGNORED,
				image,
				{
					region.imageSubresource.aspectMask,
					region.imageSubresource.mipLevel,
					1,
					region.imageSubresource.baseArrayLayer,
					region.imageSubresource.layerCount }
			};
			if (imb_from.isNeeded)
				vkCmdPipelineBarrier(commandBuffer, imb_from.stage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			//Copy
			vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			//Post-copy barrier
			if (imb_to.isNeeded) {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.dstAccessMask = imb_to.access;
				imageMemoryBarrier.newLayout = imb_to.layout;
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, imb_to.stage, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}
		}
		static void CmdBlitImage(VkCommandBuffer commandBuffer, VkImage image_src, VkImage image_dst, const VkImageBlit& region,
			imageMemoryBarrierParameterPack imb_dst_from, imageMemoryBarrierParameterPack imb_dst_to, VkFilter filter = VK_FILTER_LINEAR) {
			//Pre-blit barrier
			VkImageMemoryBarrier imageMemoryBarrier = {
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				imb_dst_from.access,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				imb_dst_from.layout,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				image_dst,
				{
					region.dstSubresource.aspectMask,
					region.dstSubresource.mipLevel,
					1,
					region.dstSubresource.baseArrayLayer,
					region.dstSubresource.layerCount }
			};
			if (imb_dst_from.isNeeded)
				vkCmdPipelineBarrier(commandBuffer, imb_dst_from.stage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			//Blit
			vkCmdBlitImage(commandBuffer,
				image_src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image_dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &region, filter);
			//Post-blit barrier
			if (imb_dst_to.isNeeded) {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.dstAccessMask = imb_dst_to.access;
				imageMemoryBarrier.newLayout = imb_dst_to.layout;
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, imb_dst_to.stage, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}
		}
		static void CmdGenerateMipmap2d(VkCommandBuffer commandBuffer, VkImage image, VkExtent2D imageExtent, uint32_t mipLevelCount, uint32_t layerCount,
			imageMemoryBarrierParameterPack imb_to, VkFilter minFilter = VK_FILTER_LINEAR) {
			auto MipmapExtent = [](VkExtent2D imageExtent, uint32_t mipLevel) {
				VkOffset3D extent = { int32_t(imageExtent.width >> mipLevel), int32_t(imageExtent.height >> mipLevel), 1 };
				extent.x += !extent.x;
				extent.y += !extent.y;
				return extent;
			};
			//Blit
			if (layerCount > 1) {
				std::unique_ptr<VkImageBlit[]> regions = std::make_unique<VkImageBlit[]>(layerCount);
				for (uint32_t i = 1; i < mipLevelCount; i++) {
					VkOffset3D mipmapExtent_src = MipmapExtent(imageExtent, i - 1);
					VkOffset3D mipmapExtent_dst = MipmapExtent(imageExtent, i);
					for (uint32_t j = 0; j < layerCount; j++)
						regions[j] = {
							{ VK_IMAGE_ASPECT_COLOR_BIT, i - 1, j, 1 },	//srcSubresource
							{ {}, mipmapExtent_src },					//srcOffsets
							{ VK_IMAGE_ASPECT_COLOR_BIT, i, j, 1 },		//dstSubresource
							{ {}, mipmapExtent_dst }					//dstOffsets
						};
					//Pre-blit barrier
					VkImageMemoryBarrier imageMemoryBarrier = {
						VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						nullptr,
						0,
						VK_ACCESS_TRANSFER_WRITE_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						VK_QUEUE_FAMILY_IGNORED,
						VK_QUEUE_FAMILY_IGNORED,
						image,
						{ VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, layerCount }
					};
					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
						0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
					//Blit
					vkCmdBlitImage(commandBuffer,
						image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						layerCount, regions.get(), minFilter);
					//Post-blit barrier
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
						0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
				}
			}
			else
				for (uint32_t i = 1; i < mipLevelCount; i++) {
					VkImageBlit region = {
						{ VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, layerCount },//srcSubresource
						{ {}, MipmapExtent(imageExtent, i - 1) },			//srcOffsets
						{ VK_IMAGE_ASPECT_COLOR_BIT, i, 0, layerCount },	//dstSubresource
						{ {}, MipmapExtent(imageExtent, i) }				//dstOffsets
					};
					CmdBlitImage(commandBuffer, image, image, region,
						{ VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED },
						{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }, minFilter);
				}
			//Post-blit barrier
			if (imb_to.isNeeded) {
				VkImageMemoryBarrier imageMemoryBarrier = {
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					nullptr,
					0,
					imb_to.access,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					imb_to.layout,
					VK_QUEUE_FAMILY_IGNORED,
					VK_QUEUE_FAMILY_IGNORED,
					image,
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, layerCount }
				};
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, imb_to.stage, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}
		}
	};

	class texture {
	protected:
		imageView imageView;
		imageMemory imageMemory;
		//--------------------
		texture() = default;
		void CreateImageMemory(VkImageType imageType, VkFormat format, VkExtent3D extent, uint32_t mipLevelCount, uint32_t arrayLayerCount, VkImageCreateFlags flags = 0) {
			VkImageCreateInfo imageCreateInfo = {
				.flags = flags,
				.imageType = imageType,
				.format = format,
				.extent = extent,
				.mipLevels = mipLevelCount,
				.arrayLayers = arrayLayerCount,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			};
			imageMemory.Create(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}
		void CreateImageView(VkImageViewType viewType, VkFormat format, uint32_t mipLevelCount, uint32_t arrayLayerCount, VkImageViewCreateFlags flags = 0) {
			imageView.Create(imageMemory.Image(), viewType, format, { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, arrayLayerCount }, flags);
		}
		//Static Function
		static std::unique_ptr<uint8_t[]> LoadFile_Internal(const auto* address, size_t fileSize, VkExtent2D& extent, VkFormat requiredFormat) {
			int componentSize = GetComponentSize(requiredFormat);
			int channelCount = GetComponentCount(requiredFormat);

			int& width = reinterpret_cast<int&>(extent.width);
			int& height = reinterpret_cast<int&>(extent.height);
			int channels;
			void* pImageData = nullptr;
			if constexpr (std::same_as<decltype(address), const char*>) {
				if (componentSize == 1)
					pImageData = stbi_load(address, &width, &height, &channels, channelCount);
				else if(componentSize == 2)
					pImageData = stbi_load_16(address, &width, &height, &channels, channelCount);
				else
					pImageData = stbi_loadf(address, &width, &height, &channels, channelCount);
				if (!pImageData)
					std::cout << std::format("[ texture ] ERROR\nFailed to load the file: {}\n", address);
			}
			if constexpr (std::same_as<decltype(address), const uint8_t*>) {
				if (fileSize > INT32_MAX) {
					std::cout << std::format("[ texture ] ERROR\nFailed to load image data from the given address! Data size must be less than 2G!\n");
					return {};
				}

				if (componentSize == 1)
					pImageData = stbi_load_from_memory(address, static_cast<int>(fileSize), &width, &height, &channels, channelCount);
				else if (componentSize == 2)
					pImageData = stbi_load_16_from_memory(address, static_cast<int>(fileSize), &width, &height, &channels, channelCount);
				else
					pImageData = stbi_loadf_from_memory(address, static_cast<int>(fileSize), &width, &height, &channels, channelCount);
				if (!pImageData)
					std::cout << std::format("[ texture ] ERROR\nFailed to load image data from the given address!\n");
			}
			return std::unique_ptr<uint8_t[]>(static_cast<uint8_t*>(pImageData));
		}
	public:
		//Getter
		VkImageView ImageView() const { return imageView; }
		VkImage Image() const { return imageMemory.Image(); }
		const VkImageView* AddressOfImageView() const { return imageView.Address(); }
		const VkImage* AddressOfImage() const { return imageMemory.AddressOfImage(); }
		//Const Function
		VkDescriptorImageInfo DescriptorImageInfo(VkSampler sampler) const {
			return { sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		}
		//Static Function	
		[[nodiscard]]
		static std::unique_ptr<uint8_t[]> LoadFile(const char* filepath, VkExtent2D& extent, VkFormat requiredFormat) {
			return LoadFile_Internal(filepath, 0, extent, requiredFormat);
		}
		[[nodiscard]]
		static std::unique_ptr<uint8_t[]> LoadFile(const uint8_t* fileBinaries, size_t fileSize, VkExtent2D& extent, VkFormat requiredFormat) {
			return LoadFile_Internal(fileBinaries, fileSize, extent, requiredFormat);
		}
		static uint32_t CalculateMipLevelCount(VkExtent2D extent) {
			return uint32_t(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
		}
		static void CopyBlitAndGenerateMipmap2d(VkBuffer buffer_copyFrom, VkImage image_copyTo, VkImage image_blitTo, VkExtent2D imageExtent,
			uint32_t mipLevelCount = 1, uint32_t layerCount = 1, VkFilter minFilter = VK_FILTER_LINEAR) {
			static constexpr imageOperation::imageMemoryBarrierParameterPack imbs[2] = {
				{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
				{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }
			};
			bool generateMipmap = mipLevelCount > 1;
			bool blitMipLevel0 = image_copyTo != image_blitTo;
			std::shared_ptr<commandBuffer> commandBuffer = Application::Get().GetCommandBufferTransfer();
			commandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			VkBufferImageCopy region = {
				.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
				.imageExtent = { imageExtent.width, imageExtent.height, 1 }
			};
			imageOperation::CmdCopyBufferToImage(commandBuffer->getHandle(), buffer_copyFrom, image_copyTo, region,
				{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED }, imbs[generateMipmap || blitMipLevel0]);
			//Blit to another image if necessary
			if (blitMipLevel0) {
				VkImageBlit region = {
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
					{ {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } },
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
					{ {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } }
				};
				imageOperation::CmdBlitImage(commandBuffer->getHandle(), image_copyTo, image_blitTo, region,
					{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED }, imbs[generateMipmap], minFilter);
			}
			//Generate mipmap if necessary, transition layout
			if (generateMipmap)
				imageOperation::CmdGenerateMipmap2d(commandBuffer->getHandle(), image_blitTo, imageExtent, mipLevelCount, layerCount,
					{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, minFilter);
			commandBuffer->End();
			//Submit
			ExecuteCommandBuffer_Graphics(commandBuffer->getHandle());
		}
		static void BlitAndGenerateMipmap2d(VkImage image_preinitialized, VkImage image_final, VkExtent2D imageExtent,
			uint32_t mipLevelCount = 1, uint32_t layerCount = 1, VkFilter minFilter = VK_FILTER_LINEAR) {
			static constexpr imageOperation::imageMemoryBarrierParameterPack imbs[2] = {
				{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
				{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }
			};
			bool generateMipmap = mipLevelCount > 1;
			bool blitMipLevel0 = image_preinitialized != image_final;
			if (generateMipmap || blitMipLevel0) {
				std::shared_ptr<commandBuffer> commandBuffer = Application::Get().GetCommandBufferTransfer();
				commandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
				//Blit to another image if necessary
				if (blitMipLevel0) {
					VkImageMemoryBarrier imageMemoryBarrier = {
						VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						nullptr,
						0,
						VK_ACCESS_TRANSFER_READ_BIT,
						VK_IMAGE_LAYOUT_PREINITIALIZED,
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						VK_QUEUE_FAMILY_IGNORED,
						VK_QUEUE_FAMILY_IGNORED,
						image_preinitialized,
						{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layerCount }
					};
					vkCmdPipelineBarrier(commandBuffer->getHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
						0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
					VkImageBlit region = {
						{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
						{ {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } },
						{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
						{ {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } }
					};
					imageOperation::CmdBlitImage(commandBuffer->getHandle(), image_preinitialized, image_final, region,
						{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED }, imbs[generateMipmap], minFilter);
				}
				//Generate mipmap if necessary, transition layout
				if (generateMipmap)
					imageOperation::CmdGenerateMipmap2d(commandBuffer->getHandle(), image_final, imageExtent, mipLevelCount, layerCount,
						{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, minFilter);
				commandBuffer->End();
				//Submit
				ExecuteCommandBuffer_Graphics(commandBuffer->getHandle());
			}
		}
		static VkSamplerCreateInfo SamplerCreateInfo() {
			return {
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.magFilter = VK_FILTER_LINEAR,
				.minFilter = VK_FILTER_LINEAR,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
				.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
				.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
				.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
				.mipLodBias = 0.f,
				.anisotropyEnable = VK_TRUE,
				.maxAnisotropy = Application::GetPhysicalDeviceProperties().limits.maxSamplerAnisotropy,
				.compareEnable = VK_FALSE,
				.compareOp = VK_COMPARE_OP_ALWAYS,
				.minLod = 0.f,
				.maxLod = VK_LOD_CLAMP_NONE,
				.borderColor = {},
				.unnormalizedCoordinates = VK_FALSE
			};
		}
	};

	class texture2d :public texture {
	protected:
		VkExtent2D extent = {};
		//--------------------
		void Create_Internal(VkFormat format_initial, VkFormat format_final, bool generateMipmap) {
			uint32_t mipLevelCount = generateMipmap ? CalculateMipLevelCount(extent) : 1;
			//Create image and allocate memory
			CreateImageMemory(VK_IMAGE_TYPE_2D, format_final, { extent.width, extent.height, 1 }, mipLevelCount, 1);
			//Create view
			CreateImageView(VK_IMAGE_VIEW_TYPE_2D, format_final, mipLevelCount, 1);
			//Copy data and generate mipmap
			if (format_initial == format_final)
				CopyBlitAndGenerateMipmap2d(stagingBuffer::Buffer_MainThread(), imageMemory.Image(), imageMemory.Image(), extent, mipLevelCount, 1);
			else
				if (VkImage image_conversion = stagingBuffer::AliasedImage2d_MainThread(format_initial, extent))
					BlitAndGenerateMipmap2d(image_conversion, imageMemory.Image(), extent, mipLevelCount, 1);
				else {
					VkImageCreateInfo imageCreateInfo = {
						.imageType = VK_IMAGE_TYPE_2D,
						.format = format_initial,
						.extent = { extent.width, extent.height, 1 },
						.mipLevels = 1,
						.arrayLayers = 1,
						.samples = VK_SAMPLE_COUNT_1_BIT,
						.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
					};
					Celestiq::Vulkan::imageMemory imageMemory_conversion(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
					CopyBlitAndGenerateMipmap2d(stagingBuffer::Buffer_MainThread(), imageMemory_conversion.Image(), imageMemory.Image(), extent, mipLevelCount, 1);
				}
		}
	public:
		texture2d() = default;
		texture2d(const char* filepath, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			Create(filepath, format_initial, format_final, generateMipmap);
		}
		texture2d(const uint8_t* pImageData_compressed, size_t fileSize, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			Create(pImageData_compressed, fileSize, format_initial, format_final, generateMipmap);
		}
		texture2d(const uint8_t* pImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			Create(pImageData, extent, format_initial, format_final, generateMipmap);
		}
		//Getter
		VkExtent2D Extent() const { return extent; }
		uint32_t Width() const { return extent.width; }
		uint32_t Height() const { return extent.height; }
		//Non-const Function
		void Create(const char* filepath, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			VkExtent2D extent;
			std::unique_ptr<uint8_t[]> pImageData = LoadFile(filepath, extent, format_initial);
			if (pImageData)
				Create(pImageData.get(), extent, format_initial, format_final, generateMipmap);
		}
		void Create(const uint8_t* pImageData_compressed, size_t fileSize, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			VkExtent2D extent;
			std::unique_ptr<uint8_t[]> pImageData = LoadFile(pImageData_compressed, fileSize, extent, format_initial);
			if (pImageData)
				Create(pImageData.get(), extent, format_initial, format_final, generateMipmap);
		}
		void Create(const uint8_t* pImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			this->extent = extent;
			//Copy data to staging buffer
			size_t imageDataSize = size_t(GetPixelOrBlockSize(format_initial)) * extent.width * extent.height;
			stagingBuffer::BufferData_MainThread(pImageData, imageDataSize);
			//Create image and allocate memory, create image view, then copy data from staging buffer to image
			Create_Internal(format_initial, format_final, generateMipmap);
		}
	};
}