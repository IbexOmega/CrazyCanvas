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
    struct TextureViewDesc;
	struct PipelineLayoutDesc;
	struct DescriptorHeapDesc;
    struct DeviceAllocatorDesc;
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
    class ICommandQueue;
	class IDescriptorSet;
	class IPipelineState;
	class IDescriptorHeap;
	class IPipelineLayout;
    class IDeviceAllocator;
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

	struct GraphicsDeviceFeatureDesc
	{
		uint32	MaxComputeWorkGroupSize[3];
		bool	RayTracing;
		bool	MeshShaders;
		bool	GeometryShaders;
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

		virtual void QueryDeviceFeatures(GraphicsDeviceFeatureDesc* pFeatures) const = 0;

		virtual IPipelineLayout* CreatePipelineLayout(const PipelineLayoutDesc* pDesc) const = 0;
		virtual IDescriptorHeap* CreateDescriptorHeap(const DescriptorHeapDesc* pDesc) const = 0;

		virtual IDescriptorSet*	CreateDescriptorSet(const char* pName, const IPipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex, IDescriptorHeap* pDescriptorHeap) const = 0;

		virtual IRenderPass*  CreateRenderPass(const RenderPassDesc* pDesc)	 const = 0;
		virtual ITextureView* CreateTextureView(const TextureViewDesc* pDesc) const = 0;
		
		virtual IShader* CreateShader(const ShaderDesc* pDesc)	const = 0;

		virtual IBuffer*  CreateBuffer(const BufferDesc* pDesc, IDeviceAllocator* pAllocator)	const = 0;
		virtual ITexture* CreateTexture(const TextureDesc* pDesc, IDeviceAllocator* pAllocator)	const = 0;
		virtual ISampler* CreateSampler(const SamplerDesc* pDesc)	const = 0;

        virtual ISwapChain*	CreateSwapChain(const Window* pWindow, ICommandQueue* pCommandQueue, const SwapChainDesc* pDesc)	const = 0;

		virtual IPipelineState*	CreateGraphicsPipelineState(const GraphicsPipelineStateDesc* pDesc) 	const = 0;
		virtual IPipelineState*	CreateComputePipelineState(const ComputePipelineStateDesc* pDesc) 	    const = 0;
		virtual IPipelineState*	CreateRayTracingPipelineState(const RayTracingPipelineStateDesc* pDesc) const = 0;
		
		virtual IAccelerationStructure*	CreateAccelerationStructure(const AccelerationStructureDesc* pDesc, IDeviceAllocator* pAllocator) const = 0;
		
		virtual ICommandQueue*		CreateCommandQueue(const char* pName, ECommandQueueType queueType)			    const = 0;
		virtual ICommandAllocator*	CreateCommandAllocator(const char* pName, ECommandQueueType queueType)		    const = 0;
		virtual ICommandList*		CreateCommandList(ICommandAllocator* pAllocator, const CommandListDesc* pDesc)  const = 0;
		virtual IFence*				CreateFence(const FenceDesc* pDesc)											    const = 0;

        virtual IDeviceAllocator* CreateDeviceAllocator(const DeviceAllocatorDesc* pDesc) const = 0;
        
		virtual void CopyDescriptorSet(const IDescriptorSet* pSrc, IDescriptorSet* pDst)																			const = 0;
		virtual void CopyDescriptorSet(const IDescriptorSet* pSrc, IDescriptorSet* pDst, const CopyDescriptorBindingDesc* pCopyBindings, uint32 copyBindingCount)	const = 0;

		/*
		* Releases the graphicsdevice. Unlike all other graphics interfaces, the graphicsdevice
		* is not referencecounted. This means that a call to release will delete the graphicsdevice. This 
		* should not be done while there still are objects alive that were created by the device.
		*/
		virtual void Release() = 0;
	};

	LAMBDA_API IGraphicsDevice* CreateGraphicsDevice(EGraphicsAPI api, const GraphicsDeviceDesc* pDesc);
}
