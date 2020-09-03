#pragma once
#include "GraphicsTypes.h"

#include "Containers/String.h"

namespace LambdaEngine
{
	struct FenceDesc;
	struct ShaderDesc;
	struct BufferDesc;
	struct TextureDesc;
	struct SamplerDesc;
	struct SwapChainDesc;
	struct QueryHeapDesc;
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
	class Fence;
	class Fence;
	class Shader;
	class Buffer;
	class Sampler;
	class Texture;
	class SwapChain;
	class QueryHeap;
	class RenderPass;
	class TextureView;
	class CommandList;
	class CommandQueue;
	class DescriptorSet;
	class PipelineState;
	class DescriptorHeap;
	class PipelineLayout;
	class DeviceAllocator;
	class CommandAllocator;
	class AccelerationStructure;

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
		String	DebugName	= "";
		bool	Debug		= false;
	};

	class GraphicsDevice
	{
	public:
		DECL_DEVICE_INTERFACE(GraphicsDevice);

		virtual QueryHeap* CreateQueryHeap(const QueryHeapDesc* pDesc) const = 0;

		virtual PipelineLayout*	CreatePipelineLayout(const PipelineLayoutDesc* pDesc) const = 0;
		virtual DescriptorHeap*	CreateDescriptorHeap(const DescriptorHeapDesc* pDesc) const = 0;

		virtual DescriptorSet*	CreateDescriptorSet(const String& debugName, const PipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex, DescriptorHeap* pDescriptorHeap) const = 0;

		virtual RenderPass*		CreateRenderPass(const RenderPassDesc* pDesc)	 const = 0;
		virtual TextureView*	CreateTextureView(const TextureViewDesc* pDesc) const = 0;
		
		virtual Shader* CreateShader(const ShaderDesc* pDesc)	const = 0;

		virtual Buffer*		CreateBuffer(const BufferDesc* pDesc, DeviceAllocator* pAllocator)		const = 0;
		virtual Texture*	CreateTexture(const TextureDesc* pDesc, DeviceAllocator* pAllocator)	const = 0;
		virtual Sampler*	CreateSampler(const SamplerDesc* pDesc)	const = 0;

		virtual SwapChain*	CreateSwapChain(const SwapChainDesc* pDesc)	const = 0;

		virtual PipelineState*	CreateGraphicsPipelineState(const GraphicsPipelineStateDesc* pDesc)										const = 0;
		virtual PipelineState*	CreateComputePipelineState(const ComputePipelineStateDesc* pDesc) 										const = 0;
		virtual PipelineState*	CreateRayTracingPipelineState(CommandQueue* pCommandQueue, const RayTracingPipelineStateDesc* pDesc)	const = 0;
		
		virtual AccelerationStructure*	CreateAccelerationStructure(const AccelerationStructureDesc* pDesc, DeviceAllocator* pAllocator) const = 0;
		
		virtual CommandQueue*		CreateCommandQueue(const String& debugName, ECommandQueueType queueType)		const = 0;
		virtual CommandAllocator*	CreateCommandAllocator(const String& debugName, ECommandQueueType queueType)	const = 0;
		virtual CommandList*		CreateCommandList(CommandAllocator* pAllocator, const CommandListDesc* pDesc)	const = 0;
		virtual Fence*				CreateFence(const FenceDesc* pDesc)												const = 0;

		virtual DeviceAllocator* CreateDeviceAllocator(const DeviceAllocatorDesc* pDesc) const = 0;
		
		virtual void CopyDescriptorSet(const DescriptorSet* pSrc, DescriptorSet* pDst)																			const = 0;
		virtual void CopyDescriptorSet(const DescriptorSet* pSrc, DescriptorSet* pDst, const CopyDescriptorBindingDesc* pCopyBindings, uint32 copyBindingCount)	const = 0;
		
		virtual void QueryDeviceFeatures(GraphicsDeviceFeatureDesc* pFeatures) const = 0;

		/*
		* Releases the graphicsdevice. Unlike all other graphics interfaces, the graphicsdevice
		* is not referencecounted. This means that a call to release will delete the graphicsdevice. This 
		* should not be done while there still are objects alive that were created by the device.
		*/
		virtual void Release() = 0;

		FORCEINLINE const GraphicsDeviceDesc& GetDesc() const
		{
			return m_Desc;
		}

	protected:
		GraphicsDeviceDesc m_Desc;
	};

	LAMBDA_API GraphicsDevice* CreateGraphicsDevice(EGraphicsAPI api, const GraphicsDeviceDesc* pDesc);
}
