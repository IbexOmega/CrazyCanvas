#pragma once
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	struct BufferDesc;
	struct TextureDesc;
    struct SwapChainDesc;
	struct ComputePipelineDesc;
	struct GraphicsPipelineDesc;
	struct RayTracingPipelineDesc;
	struct TopLevelAccelerationStructureDesc;
	struct BottomLevelAccelerationStructureDesc;

    class Window;
	class IFence;
	class ICommandQueue;
	class IFence;
	class IBuffer;
	class ITexture;
    class ISwapChain;
	class IRenderPass;
	class ITextureView;
	class ICommandList;
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
		bool Debug;
	};

	class IGraphicsDevice
	{
	public:
		DECL_INTERFACE(IGraphicsDevice);

		virtual bool Init(const GraphicsDeviceDesc& desc) 	= 0;
		virtual void Release() 								= 0;

		virtual IRenderPass*						CreateRenderPass()																			const = 0;
		virtual IBuffer*							CreateBuffer(const BufferDesc& desc)														const = 0;
		virtual ITexture*							CreateTexture(const TextureDesc& desc)														const = 0;
		virtual ITextureView*						CreateTextureView()																			const = 0;
        virtual ISwapChain*							CreateSwapChain(const Window* pWindow, const SwapChainDesc& desc)							const = 0;
		virtual IPipelineState*						CreateGraphicsPipelineState(const GraphicsPipelineDesc& desc) 								const = 0;
		virtual IPipelineState*						CreateComputePipelineState(const ComputePipelineDesc& desc) 								const = 0;
		virtual IPipelineState*						CreateRayTracingPipelineState(const RayTracingPipelineDesc& desc)							const = 0;
		virtual ITopLevelAccelerationStructure*		CreateTopLevelAccelerationStructure(const TopLevelAccelerationStructureDesc& desc)			const = 0;
		virtual IBottomLevelAccelerationStructure*	CreateBottomLevelAccelerationStructure(const BottomLevelAccelerationStructureDesc& desc)	const = 0;
		virtual ICommandList*						CreateCommandList(ICommandAllocator* pAllocator, ECommandListType commandListType)			const = 0;
		virtual ICommandAllocator*					CreateCommandAllocator(EQueueType queueType)												const = 0;
		virtual ICommandQueue*								CreateCommandQueue(EQueueType queueType)															const = 0;
		virtual IFence*								CreateFence(uint64 initalValue)																const = 0;
	};

	LAMBDA_API IGraphicsDevice* CreateGraphicsDevice(const GraphicsDeviceDesc& desc, EGraphicsAPI api);
}
