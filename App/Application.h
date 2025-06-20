#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

#define IMGUI_ENABLE_DOCKING      // 启用 Docking
#define IMGUI_ENABLE_VIEWPORTS    // 启用 Multi-Viewport
#include "imgui.h"
#include "vulkan/vulkan.h"

void check_vk_result(VkResult err);

struct GLFWwindow;

namespace Celestiq::Vulkan {
	class commandBuffer;
	class commandPool;
}

namespace Celestiq {
    class Layer
	{
	public:
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void OnStart() {}
		virtual void OnUpdate(float ts) {}
		virtual void OnUIRender() {}
	};

	struct ApplicationSpecification
	{
		std::string Name = "Celestiq Renderer";
		uint32_t Width = 1600;
		uint32_t Height = 900;
	};

	class Application
	{
	public:
		Application(const ApplicationSpecification& applicationSpecification = ApplicationSpecification());
		~Application();

		static Application& Get();

		void Run();
		void SetMenubarCallback(const std::function<void()>& menubarCallback) { m_MenubarCallback = menubarCallback; }
		
		template<typename T>
		void PushLayer()
		{
			static_assert(std::is_base_of<Layer, T>::value, "Pushed type is not subclass of Layer!");
			m_LayerStack.emplace_back(std::make_shared<T>())->OnAttach();
		}

		void PushLayer(const std::shared_ptr<Layer>& layer) { m_LayerStack.emplace_back(layer); layer->OnAttach(); }

		void Close();

		float GetTime();
		GLFWwindow* GetWindowHandle() const { return m_WindowHandle; }

		// getter
		static const VkInstance GetInstance();
		static const VkPhysicalDevice GetPhysicalDevice();
		static const VkDevice GetDevice();
		static const VkQueue GetQueue();
		static const VkPhysicalDeviceMemoryProperties& GetPhysicalDeviceMemoryProperties();
		static const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties();
		static const uint32_t GetQueueFamilyIndex();

		const std::shared_ptr<Vulkan::commandPool> GetCommandPoolGraphics();
		const std::shared_ptr<Vulkan::commandPool> GetCommandPoolCompute();
		const std::shared_ptr<Vulkan::commandBuffer> GetCommandBufferTransfer();

		static void SubmitResourceFree(std::function<void()>&& func);

		void AddCallback_DestroyDevice(void(*function)()) {
			callbacks_destroyDevice.push_back(function);
		}
	private:
		void Init();
		void Shutdown();
	private:
		std::vector<void(*)()> callbacks_destroyDevice;

		ApplicationSpecification m_Specification;
		GLFWwindow* m_WindowHandle = nullptr;
		bool m_Running = false;

		float m_TimeStep = 0.0f;
		float m_FrameTime = 0.0f;
		float m_LastFrameTime = 0.0f;

		std::vector<std::shared_ptr<Layer>> m_LayerStack;
		std::function<void()> m_MenubarCallback;

		std::shared_ptr<Vulkan::commandPool> g_commandPool_graphics;
		std::shared_ptr<Vulkan::commandPool> g_commandPool_compute;
		std::shared_ptr<Vulkan::commandBuffer> g_commandBuffer_transfer;
	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);
}