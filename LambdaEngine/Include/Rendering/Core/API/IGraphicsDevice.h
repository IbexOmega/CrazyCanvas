#pragma once
#include "GraphicsTypes.h"

namespace LambdaEngine
{
    struct FenceDesc;
    struct BufferDesc;
    struct TextureDesc;
    struct SwapChainDesc;
	struct RenderPassDesc;
    struct CommandListDesc;
    struct TextureViewDesc;
	struct FrameBufferDesc;
    struct ComputePipelineDesc;
    struct GraphicsPipelineDesc;
    struct RayTracingPipelineDesc;
    struct TopLevelAccelerationStructureDesc;
    struct BottomLevelAccelerationStructureDesc;

    class Window;
	class IFence;
    class IFence;
    class IBuffer;
    class ITexture;
    class ISwapChain;
    class IRenderPass;
    class ITextureView;
    class ICommandList;
	class IFrameBuffer;
    class ICommandQueue;
	class IPipelineState;
	class ICommandAllocator;
	class ITopLevelAccelerationStructure;
	class IBottomLevelAccelerationStructure;

	enum class EGraphicsAPI
	{
		VULKAN = 0,
	};

	struct GraphicsDeviceDesc
	{
		const char* pName = "";
		bool		Debug;
	};

	class IGraphicsDevice
	{
	public:
		DECL_DEVICE_INTERFACE(IGraphicsDevice);

		virtual IFrameBuffer* CreateFrameBuffer(IRenderPass* pRenderPass, const FrameBufferDesc& desc)	const = 0;
		virtual IRenderPass*  CreateRenderPass(const RenderPassDesc& desc)								const = 0;
		virtual ITextureView* CreateTextureView(const TextureViewDesc& desc)							const = 0;
		
		virtual IBuffer*	CreateBuffer(const BufferDesc& desc)								const = 0;
		virtual ITexture*	CreateTexture(const TextureDesc& desc)								const = 0;
        virtual ISwapChain*	CreateSwapChain(const Window* pWindow, const SwapChainDesc& desc)	const = 0;

		virtual IPipelineState*	CreateGraphicsPipelineState(const GraphicsPipelineDesc& desc) 	  const = 0;
		virtual IPipelineState*	CreateComputePipelineState(const ComputePipelineDesc& desc) 	  const = 0;
		virtual IPipelineState*	CreateRayTracingPipelineState(const RayTracingPipelineDesc& desc) const = 0;
		
		virtual ITopLevelAccelerationStructure*		CreateTopLevelAccelerationStructure(const TopLevelAccelerationStructureDesc& desc)			const = 0;
		virtual IBottomLevelAccelerationStructure*	CreateBottomLevelAccelerationStructure(const BottomLevelAccelerationStructureDesc& desc)	const = 0;
		
		virtual ICommandQueue*		CreateCommandQueue(ECommandQueueType queueType)								  const = 0;
		virtual ICommandAllocator*	CreateCommandAllocator(ECommandQueueType queueType)							  const = 0;
		virtual ICommandList*		CreateCommandList(ICommandAllocator* pAllocator, const CommandListDesc& desc) const = 0;
		virtual IFence*				CreateFence(const FenceDesc& desc)											  const = 0;

		/*
		* Releases the graphicsdevice. Unlike all other graphics interfaces, the graphicsdevice
		* is not referencecounted. This means that a call to release will delete the graphicsdevice. This 
		* should not be done while there still are objects alive that were created by the device.
		*/
		virtual void Release() = 0;
	};

	LAMBDA_API IGraphicsDevice* CreateGraphicsDevice(EGraphicsAPI api, const GraphicsDeviceDesc& desc);
}
