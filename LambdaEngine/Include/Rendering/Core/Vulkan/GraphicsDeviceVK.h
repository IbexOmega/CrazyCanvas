#pragma once
#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Utilities/StringHash.h"
#include "Vulkan.h"

#include <vector>

namespace LambdaEngine
{
	typedef ConstString ValidationLayer;
	typedef ConstString Extension;

	class GraphicsDeviceVK : public IGraphicsDevice
	{
	public:
		GraphicsDeviceVK();
		~GraphicsDeviceVK();

		virtual bool Init(const GraphicsDeviceDesc& desc) override;
		virtual void Release() override;

		//CREATE
		virtual IRenderPass* CreateRenderPass() override;
		virtual IFence* CreateFence() override;
		virtual ICommandList* CreateCommandList() override;
		virtual IPipelineState* CreatePipelineState() override;
		virtual IBuffer* CreateBuffer() override;
		virtual ITexture* CreateTexture() override;
		virtual ITextureView* CreateTextureView() override;

	private:
		//INIT
		bool InitInstance(const GraphicsDeviceDesc& desc);

		bool InitDevice();
		bool InitPhysicalDevice();
		bool InitLogicalDevice();

		//UTIL
		bool SetEnabledValidationLayers();
		bool SetEnabledInstanceExtensions();
		void RegisterExtensionFunctions();

		bool IsInstanceExtensionEnabled(const char* extensionName);
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

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

		PFN_vkSetDebugUtilsObjectNameEXT	vkSetDebugUtilsObjectNameEXT;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
		PFN_vkCreateDebugUtilsMessengerEXT	vkCreateDebugUtilsMessengerEXT;

	private:
		VkDebugUtilsMessengerEXT m_DebugMessenger;

		std::vector<const char*> m_EnabledValidationLayers;
		std::vector<const char*> m_EnabledInstanceExtensions;
	};
}