#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	struct GraphicsPipelineDesc;
	struct ComputePipelineDesc;
	struct RayTracePipelineDesc;
	struct BufferDesc;

	class IRenderPass;
	class IFence;
	class ICommandList;
	class IPipelineState;
	class IBuffer;
	class ITexture;
	class ITextureView;

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

		virtual bool Init(const GraphicsDeviceDesc& desc) = 0;
		virtual void Release() = 0;

		virtual IRenderPass*		CreateRenderPass() = 0;
		virtual IFence*				CreateFence() = 0;
		virtual ICommandList*		CreateCommandList() = 0;
		virtual IPipelineState*		CreateGraphicsPipelineState(const GraphicsPipelineDesc& desc) = 0;
		virtual IPipelineState*		CreateComputePipelineState(const ComputePipelineDesc& desc) = 0;
		virtual IPipelineState*		CreateRayTracePipelineState(const RayTracePipelineDesc& desc) = 0;
		virtual IBuffer*			CreateBuffer(const BufferDesc& desc) = 0;
		virtual ITexture*			CreateTexture() = 0;
		virtual ITextureView*		CreateTextureView() = 0;
	};

	LAMBDA_API IGraphicsDevice* CreateGraphicsDevice(const GraphicsDeviceDesc& desc, EGraphicsAPI api);
}