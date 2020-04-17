#pragma once
#include "GraphicsTypes.h"

namespace LambdaEngine
{
    struct FenceDesc;
	struct ShaderDesc;
    struct BufferDesc;
    struct TextureDesc;
	struct SamplerDesc;
    struct SwapChainDesc;
	struct RenderPassDesc;
    struct CommandListDesc;
	struct FrameBufferDesc;
    struct TextureViewDesc;
	struct PipelineLayoutDesc;
	struct DescriptorHeapDesc;
    struct ComputePipelineStateDesc;
    struct GraphicsPipelineStateDesc;
	struct AccelerationStructureDesc;
    struct RayTracingPipelineStateDesc;

    class Window;
	class IFence;
    class IFence;
	class IShader;
    class IBuffer;
	class ISampler;
    class ITexture;
    class ISwapChain;
    class IRenderPass;
    class ITextureView;
    class ICommandList;
	class IFrameBuffer;
    class ICommandQueue;
	class IDescriptorSet;
	class IPipelineState;
	class IDescriptorHeap;
	class IPipelineLayout;
	class ICommandAllocator;
	class IAccelerationStructure;

	enum class EGraphicsAPI
	{
		VULKAN = 0,
	};

	struct CopyDescriptorBindingDesc
	{
		uint32 SrcBinding		= 0;
		uint32 DstBinding		= 0;
		uint32 DescriptorCount	= 0;
	};

	struct GraphicsDeviceDesc
	{
		const char* pName = "";
		bool		Debug = false;
	};

	class IGraphicsDevice
	{
	public:
		DECL_DEVICE_INTERFACE(IGraphicsDevice);

		virtual IPipelineLayout* CreatePipelineLayout(const PipelineLayoutDesc& desc) const = 0;
		virtual IDescriptorHeap* CreateDescriptorHeap(const DescriptorHeapDesc& desc) const = 0;

		virtual IDescriptorSet*	CreateDescriptorSet(const char* pName, const IPipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex, IDescriptorHeap* pDescriptorHeap) const = 0;

		virtual IFrameBuffer* CreateFrameBuffer(IRenderPass* pRenderPass, const FrameBufferDesc& desc)	const = 0;

		virtual IRenderPass*  CreateRenderPass(const RenderPassDesc& desc)	 const = 0;
		virtual ITextureView* CreateTextureView(const TextureViewDesc& desc) const = 0;
		
		virtual IShader* CreateShader(const ShaderDesc& desc)	const = 0;

		virtual IBuffer*  CreateBuffer(const BufferDesc& desc)	const = 0;
		virtual ITexture* CreateTexture(const TextureDesc& desc)	const = 0;
		virtual ISampler* CreateSampler(const SamplerDesc& desc)	const = 0;

        virtual ISwapChain*	CreateSwapChain(const Window* pWindow, ICommandQueue* pCommandQueue, const SwapChainDesc& desc)	const = 0;

		virtual IPipelineState*	CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc) 	  const = 0;
		virtual IPipelineState*	CreateComputePipelineState(const ComputePipelineStateDesc& desc) 	  const = 0;
		virtual IPipelineState*	CreateRayTracingPipelineState(const RayTracingPipelineStateDesc& desc) const = 0;
		
		virtual IAccelerationStructure*	CreateAccelerationStructure(const AccelerationStructureDesc& desc) const = 0;
		
		virtual ICommandQueue*		CreateCommandQueue(const char* pName, ECommandQueueType queueType)			  const = 0;
		virtual ICommandAllocator*	CreateCommandAllocator(const char* pName, ECommandQueueType queueType)		  const = 0;
		virtual ICommandList*		CreateCommandList(ICommandAllocator* pAllocator, const CommandListDesc& desc) const = 0;
		virtual IFence*				CreateFence(const FenceDesc& desc)											  const = 0;

		virtual void CopyDescriptorSet(const IDescriptorSet* pSrc, IDescriptorSet* pDst)																			const = 0;
		virtual void CopyDescriptorSet(const IDescriptorSet* pSrc, IDescriptorSet* pDst, const CopyDescriptorBindingDesc* pCopyBindings, uint32 copyBindingCount)	const = 0;

		/*
		* Releases the graphicsdevice. Unlike all other graphics interfaces, the graphicsdevice
		* is not referencecounted. This means that a call to release will delete the graphicsdevice. This 
		* should not be done while there still are objects alive that were created by the device.
		*/
		virtual void Release() = 0;
	};

	LAMBDA_API IGraphicsDevice* CreateGraphicsDevice(EGraphicsAPI api, const GraphicsDeviceDesc& desc);
}
