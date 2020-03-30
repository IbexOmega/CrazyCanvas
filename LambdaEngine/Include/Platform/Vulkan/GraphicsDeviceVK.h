#pragma once
#include "Rendering/Core/IGraphicsDevice.h"
#include "Vulkan.h"

#include <vector>

namespace LambdaEngine
{
	class GraphicsDeviceVK : public IGraphicsDevice
	{
	public:
		GraphicsDeviceVK(); 
		~GraphicsDeviceVK();

		virtual bool Init(const GraphicsDeviceDesc& desc) override;
		virtual void Release() override;

		//CREATE
		virtual IRenderPass*		CreateRenderPass() override;
		virtual IFence*				CreateFence() override;
		virtual ICommandList*		CreateCommandList() override;
		virtual IPipelineState*		CreatePipelineState() override;
		virtual IBuffer*			CreateBuffer() override;
		virtual ITexture*			CreateTexture() override;
		virtual ITextureView*		CreateTextureView() override;
		
	private:
		//INIT
		bool InitInstance(const GraphicsDeviceDesc& desc);

		bool InitDevice();
		bool InitPhysicalDevice();
		bool InitLogicalDevice();

		//UTIL
		bool ValidationLayersSupported();

		int32 RatePhysicalDevice(VkPhysicalDevice physicalDevice);

	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

	public:
		VkInstance Instance;

		VkPhysicalDevice PhysicalDevice;
		VkDevice Device;

	private:
		std::vector<const char*> m_ValidationLayers;

		std::vector<const char*> m_RequiredInstanceExtensions;
		std::vector<const char*> m_OptionalInstanceExtensions;
	};
}