#pragma once
#include "Containers/TArray.h"

#include "Rendering/Core/API/SwapChain.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class TextureVK;
	class CommandQueueVK;
	class GraphicsDeviceVK;

	class SwapChainVK : public TDeviceChildBase<GraphicsDeviceVK, SwapChain>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, SwapChain>;
		
	public:
		SwapChainVK(const GraphicsDeviceVK* pDevice);
		~SwapChainVK();
		
		bool Init(const SwapChainDesc* pDesc);
		
	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override final;
		
		// SwapChain interface
		virtual bool ResizeBuffers(uint32 width, uint32 height) override final;
		
		virtual bool Present() override final;
		
		virtual Texture*		GetBuffer(uint32 bufferIndex)		override final;
		virtual const Texture*	GetBuffer(uint32 bufferIndex) const	override final;
	
		FORCEINLINE virtual uint64 GetCurrentBackBufferIndex() const override final
		{
			return static_cast<uint64>(m_BackBufferIndex);
		}
		
	private:
		bool InitSwapChain(uint32 width, uint32 height);
		
		VkResult	AquireNextImage();
		void		ReleaseResources();
		
	private:
		VkSurfaceKHR	m_Surface	= VK_NULL_HANDLE;
		VkSwapchainKHR	m_SwapChain	= VK_NULL_HANDLE;
		
		uint32  m_BackBufferIndex = 0;
		uint32  m_SemaphoreIndex  = 0;

		VkPresentModeKHR	m_PresentationMode;
		VkSurfaceFormatKHR	m_VkFormat;
		
		TArray<TextureVK*>	m_Buffers;
		TArray<VkSemaphore>	m_ImageSemaphores;
		TArray<VkSemaphore>	m_RenderSemaphores;
	};
}
