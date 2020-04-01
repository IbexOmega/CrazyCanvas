#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	struct GraphicsPipelineDesc;
	struct ComputePipelineDesc;
	struct RayTracingPipelineDesc;
	struct BufferDesc;
	struct TextureDesc;
    struct SwapChainDesc;
	struct TopLevelAccelerationStructureDesc;
	struct BottomLevelAccelerationStructureDesc;

    class Window;
	class IRenderPass;
	class IFence;
	class ICommandList;
	class IPipelineState;
	class IBuffer;
	class ITexture;
    class ISwapChain;
	class ITextureView;
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
		virtual IFence*								CreateFence()																				const = 0;
		virtual ICommandList*						CreateCommandList()																			const = 0;
		virtual IBuffer*							CreateBuffer(const BufferDesc& desc)														const = 0;
		virtual ITexture*							CreateTexture(const TextureDesc& desc)														const = 0;
		virtual ITextureView*						CreateTextureView()																			const = 0;
        virtual ISwapChain*							CreateSwapChain(const Window* pWindow, const SwapChainDesc& desc)							const = 0;
		virtual IPipelineState*						CreateGraphicsPipelineState(const GraphicsPipelineDesc& desc) 								const = 0;
		virtual IPipelineState*						CreateComputePipelineState(const ComputePipelineDesc& desc) 								const = 0;
		virtual IPipelineState*						CreateRayTracingPipelineState(const RayTracingPipelineDesc& desc)							const = 0;
		virtual ITopLevelAccelerationStructure*		CreateTopLevelAccelerationStructure(const TopLevelAccelerationStructureDesc& desc)			const = 0;
		virtual IBottomLevelAccelerationStructure*	CreateBottomLevelAccelerationStructure(const BottomLevelAccelerationStructureDesc& desc)	const = 0;
	};

	LAMBDA_API IGraphicsDevice* CreateGraphicsDevice(const GraphicsDeviceDesc& desc, EGraphicsAPI api);
}
