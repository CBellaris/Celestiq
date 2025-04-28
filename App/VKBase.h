#pragma once
#include <vulkan/vulkan.h>
#include <format>
#include <iostream>
#include <cstring>
#include <span>
#include <fstream>

#include "Application.h" 

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
		const VkBuffer* Address() const { return bufferMemory.AddressOfBuffer(); }
		VkDeviceSize AllocationSize() const { return bufferMemory.AllocationSize(); }
		//Const Function
		void TransferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const {
			if (bufferMemory.MemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
				bufferMemory.BufferData(pData_src, size, offset);
				return;
			}
			stagingBuffer::BufferData_MainThread(pData_src, size);

			const commandBuffer& commandBuffer = Celestiq::Application::CommandBuffer_Transfer();
			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			VkBufferCopy region = { 0, offset, size };
			vkCmdCopyBuffer(commandBuffer, stagingBuffer::Buffer_MainThread(), bufferMemory.Buffer(), 1, &region);
			commandBuffer.End();

			fence fence;
			VkSubmitInfo submitInfo = {
				.commandBufferCount = 1,
				.pCommandBuffers = commandBuffer.Address()
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
			const commandBuffer& commandBuffer = Celestiq::Application::CommandBuffer_Transfer();
			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			std::unique_ptr<VkBufferCopy[]> regions = std::make_unique<VkBufferCopy[]>(elementCount);
			for (size_t i = 0; i < elementCount; i++)
				regions[i] = { stride_src * i, stride_dst * i + offset, elementSize };
			vkCmdCopyBuffer(commandBuffer, stagingBuffer::Buffer_MainThread(), bufferMemory.Buffer(), elementCount, regions.get());
			commandBuffer.End();
				
			fence fence;
			VkSubmitInfo submitInfo = {
				.commandBufferCount = 1,
				.pCommandBuffers = commandBuffer.Address()
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
}