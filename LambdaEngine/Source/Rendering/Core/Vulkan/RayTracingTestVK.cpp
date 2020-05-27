#include "Rendering/Core/Vulkan/RayTracingTestVK.h"
#include "Rendering/Core/Vulkan/CommandAllocatorVK.h"
#include "Rendering/Core/Vulkan/CommandListVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/ShaderVK.h"
#include "Rendering/Core/Vulkan/TextureViewVK.h"
#include "Rendering/Core/Vulkan/FenceVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/RayTracingPipelineStateVK.h"
#include "Rendering/Core/Vulkan/DescriptorHeapVK.h"
#include "Rendering/Core/Vulkan/DescriptorSetVK.h"

#include "Game/Scene.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/PipelineStateManager.h"

#include "Resources/ResourceManager.h"

#include "Math/Math.h"

#include "Log/Log.h"

#include <array>

#define INDEX_RAYGEN 0
#define INDEX_CLOSEST_HIT 1
#define INDEX_MISS 2

namespace LambdaEngine
{
	const AccelerationStructureVK*		RayTracingTestVK::s_pSceneTLAS = nullptr;

	AccelerationStructureVK*			RayTracingTestVK::s_pTLAS = nullptr;
	AccelerationStructureVK*			RayTracingTestVK::s_pBLAS = nullptr;
	
	BufferVK*							RayTracingTestVK::s_pBLASSerializedBuffer = nullptr;
	BufferVK*							RayTracingTestVK::s_pTLASSerializedBuffer = nullptr;
	
	IFence*								RayTracingTestVK::s_pFence		= nullptr;
	uint64								RayTracingTestVK::s_SignalValue	= 1;

	ITextureView*						RayTracingTestVK::s_ppTextureViews[3];

	CommandAllocatorVK*					RayTracingTestVK::s_pGraphicsPreCommandAllocators[3];
	CommandAllocatorVK*					RayTracingTestVK::s_pGraphicsPostCommandAllocators[3];
	CommandAllocatorVK*					RayTracingTestVK::s_pComputeCommandAllocators[3];
	
	CommandListVK*						RayTracingTestVK::s_pGraphicsPreCommandLists[3];
	CommandListVK*						RayTracingTestVK::s_pGraphicsPostCommandLists[3];
	CommandListVK*						RayTracingTestVK::s_pComputeCommandLists[3];

	BufferVK*							RayTracingTestVK::s_pSBT = nullptr;

	uint64								RayTracingTestVK::s_PipelineStateID	= 0;
	IPipelineLayout*					RayTracingTestVK::s_pPipelineLayout		= nullptr;

	IDescriptorHeap*					RayTracingTestVK::s_pDescriptorHeap		= nullptr;
	IDescriptorSet*						RayTracingTestVK::s_pDescriptorSets[3]	= { nullptr, nullptr, nullptr };


	void RayTracingTestVK::InitCommandLists()
	{
		const GraphicsDeviceVK* pGraphicsDeviceVk = reinterpret_cast<const GraphicsDeviceVK*>(RenderSystem::GetDevice());

		for (uint32 i = 0; i < 3; i++)
		{
			s_pGraphicsPreCommandAllocators[i] = reinterpret_cast<CommandAllocatorVK*>(pGraphicsDeviceVk->CreateCommandAllocator("RT Testing Graphics Pre Command Allocator", ECommandQueueType::COMMAND_QUEUE_GRAPHICS));
			s_pGraphicsPostCommandAllocators[i] = reinterpret_cast<CommandAllocatorVK*>(pGraphicsDeviceVk->CreateCommandAllocator("RT Testing Graphics Post Command Allocator", ECommandQueueType::COMMAND_QUEUE_GRAPHICS));
			s_pComputeCommandAllocators[i] = reinterpret_cast<CommandAllocatorVK*>(pGraphicsDeviceVk->CreateCommandAllocator("RT Testing Compute Command Allocator", ECommandQueueType::COMMAND_QUEUE_COMPUTE));

			CommandListDesc graphicsPreCommandListDesc = {};
			graphicsPreCommandListDesc.pName			= "RT Testing Graphics Pre Command List";
			graphicsPreCommandListDesc.Flags			= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
			graphicsPreCommandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_PRIMARY;

			CommandListDesc graphicsPostCommandListDesc = {};
			graphicsPostCommandListDesc.pName			= "RT Testing Graphics Post Command List";
			graphicsPostCommandListDesc.Flags			= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
			graphicsPostCommandListDesc.CommandListType = ECommandListType::COMMAND_LIST_PRIMARY;

			CommandListDesc computeCommandListDesc = {};
			computeCommandListDesc.pName			= "RT Testing Compute Command List";
			computeCommandListDesc.Flags			= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
			computeCommandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_PRIMARY;

			s_pGraphicsPreCommandLists[i]	= reinterpret_cast<CommandListVK*>(pGraphicsDeviceVk->CreateCommandList(s_pGraphicsPreCommandAllocators[i], &graphicsPreCommandListDesc));
			s_pGraphicsPostCommandLists[i]	= reinterpret_cast<CommandListVK*>(pGraphicsDeviceVk->CreateCommandList(s_pGraphicsPostCommandAllocators[i], &graphicsPostCommandListDesc));
			s_pComputeCommandLists[i]		= reinterpret_cast<CommandListVK*>(pGraphicsDeviceVk->CreateCommandList(s_pComputeCommandAllocators[i], &computeCommandListDesc));
		}
	}

	void RayTracingTestVK::InitRenderer(ITextureView** ppBackBufferTextureViews, GUID_Lambda raygenShader, GUID_Lambda closestHitShader, GUID_Lambda missShader)
	{
		const GraphicsDeviceVK* pGraphicsDeviceVk = reinterpret_cast<const GraphicsDeviceVK*>(RenderSystem::GetDevice());

		for (uint32 i = 0; i < 3; i++)
		{
			s_ppTextureViews[i] = ppBackBufferTextureViews[i];
		}

		DescriptorBindingDesc bindings[2];
		bindings[0].Binding			= 0;
		bindings[0].DescriptorCount = 1;
		bindings[0].DescriptorType	= EDescriptorType::DESCRIPTOR_ACCELERATION_STRUCTURE;
		bindings[0].ShaderStageMask = FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER;

		bindings[1].Binding			= 1;
		bindings[1].DescriptorCount = 1;
		bindings[1].DescriptorType	= EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_TEXTURE;
		bindings[1].ShaderStageMask = FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER;

		DescriptorSetLayoutDesc descriptorLayout = { };
		descriptorLayout.DescriptorBindingCount = 2;
		descriptorLayout.pDescriptorBindings	= bindings;

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.pName					= "PipelineLayout";
		pipelineLayoutDesc.DescriptorSetLayoutCount = 1;
		pipelineLayoutDesc.pDescriptorSetLayouts	= &descriptorLayout;

		PipelineLayoutVK* pPipelineLayout = reinterpret_cast<PipelineLayoutVK*>(pGraphicsDeviceVk->CreatePipelineLayout(&pipelineLayoutDesc));
		s_pPipelineLayout = pPipelineLayout;

		RayTracingManagedPipelineStateDesc pipelineDesc = { };
		pipelineDesc.pName					= "Ray Tracing Pipeline";
		pipelineDesc.RaygenShader			= raygenShader;
		pipelineDesc.MissShaderCount		= 1;
		pipelineDesc.pMissShaders[0]		= missShader;
		pipelineDesc.ClosestHitShaderCount	= 1;
		pipelineDesc.pClosestHitShaders[0]	= closestHitShader;
		pipelineDesc.MaxRecursionDepth		= 1;
		pipelineDesc.pPipelineLayout		= pPipelineLayout;

		s_PipelineStateID = PipelineStateManager::CreateRayTracingPipelineState(&pipelineDesc);

		//Create Descriptor Sets
		{
			DescriptorHeapDesc heapDesc = { };
			heapDesc.DescriptorCount.DescriptorSetCount						= 3;
			heapDesc.DescriptorCount.AccelerationStructureDescriptorCount	= 1;
			heapDesc.DescriptorCount.UnorderedAccessTextureDescriptorCount	= 1;

			s_pDescriptorHeap = pGraphicsDeviceVk->CreateDescriptorHeap(&heapDesc);
			for (uint32 i = 0; i < 3; i++)
			{
				s_pDescriptorSets[i] = pGraphicsDeviceVk->CreateDescriptorSet(nullptr, pPipelineLayout, 0, s_pDescriptorHeap);

				const IAccelerationStructure* pTLAS = s_pSceneTLAS;
				s_pDescriptorSets[i]->WriteAccelerationStructureDescriptors(&pTLAS, 0, 1);
				
				ITextureView* pTexView = ppBackBufferTextureViews[i];
				s_pDescriptorSets[i]->WriteTextureDescriptors(&pTexView, nullptr, ETextureState::TEXTURE_STATE_GENERAL, 1, 1, EDescriptorType::DESCRIPTOR_UNORDERED_ACCESS_TEXTURE);
			}
		}

		FenceDesc fenceDesc = {};
		fenceDesc.pName			= "RT Testing Fence";
		fenceDesc.InitalValue	= 0;

		s_pFence = reinterpret_cast<FenceVK*>(pGraphicsDeviceVk->CreateFence(&fenceDesc));
	}

	void RayTracingTestVK::CreateBLAS()
	{
		const GraphicsDeviceVK* pGraphicsDeviceVk = reinterpret_cast<const GraphicsDeviceVK*>(RenderSystem::GetDevice());
		
		AccelerationStructureDesc accelerationStructureDesc = {};
		accelerationStructureDesc.pName				= "BLAS";
		accelerationStructureDesc.MaxTriangleCount	= 1;
		accelerationStructureDesc.MaxVertexCount	= 3;
		accelerationStructureDesc.Type				= EAccelerationStructureType::ACCELERATION_STRUCTURE_BOTTOM;
		accelerationStructureDesc.AllowsTransform	= false;
		accelerationStructureDesc.Flags				= 0;

		s_pBLAS = reinterpret_cast<AccelerationStructureVK*>(pGraphicsDeviceVk->CreateAccelerationStructure(&accelerationStructureDesc, nullptr));
	}

	void RayTracingTestVK::BuildBLAS()
	{
		const GraphicsDeviceVK* pGraphicsDeviceVk = reinterpret_cast<const GraphicsDeviceVK*>(RenderSystem::GetDevice());

		std::vector<Vertex> vertices(3);
		vertices[0].Position	= glm::vec3(1.0f, 1.0f, 0.0f);
		vertices[0].Normal		= glm::vec3(0.0f, 0.0f, -1.0f);
		vertices[0].Tangent		= glm::vec3(1.0f, 0.0f, 0.0f);
		vertices[0].TexCoord	= glm::vec2(1.0f, 0.0f);
		vertices[1].Position	= glm::vec3(-1.0f, 1.0f, 0.0f);
		vertices[1].Normal		= glm::vec3(0.0f, 0.0f, -1.0f);
		vertices[1].Tangent		= glm::vec3(1.0f, 0.0f, 0.0f);
		vertices[1].TexCoord	= glm::vec2(0.0f, 0.0f);
		vertices[2].Position	= glm::vec3(0.0f, -1.0f, 0.0f);
		vertices[2].Normal		= glm::vec3(0.0f, 0.0f, -1.0f);
		vertices[2].Tangent		= glm::vec3(1.0f, 0.0f, 0.0f);
		vertices[2].TexCoord	= glm::vec2(0.5f, 0.5f);

		std::vector<uint32_t> indices = { 0, 1, 2 };

		glm::mat3x4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));

		std::vector<glm::mat3x4> transforms = { scale };

		auto vertex_buffer_size			= vertices.size()	* sizeof(Vertex);
		auto index_buffer_size			= indices.size()	* sizeof(uint32_t);
		auto transform_buffer_size		= transforms.size() * sizeof(glm::mat3x4);

		BufferDesc vertexBufferDesc = {};
		vertexBufferDesc.pName				= "Temp Vertex Buffer";
		vertexBufferDesc.MemoryType			= EMemoryType::MEMORY_CPU_VISIBLE;
		vertexBufferDesc.Flags				= FBufferFlags::BUFFER_FLAG_VERTEX_BUFFER;
		vertexBufferDesc.SizeInBytes		= vertex_buffer_size;

		BufferDesc indexBufferDesc = {};
		indexBufferDesc.pName				= "Temp Index Buffer";
		indexBufferDesc.MemoryType			= EMemoryType::MEMORY_CPU_VISIBLE;
		indexBufferDesc.Flags				= FBufferFlags::BUFFER_FLAG_INDEX_BUFFER;
		indexBufferDesc.SizeInBytes			= index_buffer_size;

		BufferDesc transformBufferDesc = {};
		transformBufferDesc.pName			= "Temp Transform Buffer";
		transformBufferDesc.MemoryType		= EMemoryType::MEMORY_CPU_VISIBLE;
		transformBufferDesc.Flags			= FBufferFlags::BUFFER_FLAG_NONE;
		transformBufferDesc.SizeInBytes		= transform_buffer_size;

		BufferVK* pTempVertexBuffer		= reinterpret_cast<BufferVK*>(pGraphicsDeviceVk->CreateBuffer(&vertexBufferDesc, nullptr));
		BufferVK* pTempIndexBuffer		= reinterpret_cast<BufferVK*>(pGraphicsDeviceVk->CreateBuffer(&indexBufferDesc, nullptr));
		BufferVK* pTempTransformBuffer	= reinterpret_cast<BufferVK*>(pGraphicsDeviceVk->CreateBuffer(&transformBufferDesc, nullptr));

		void* pMapped = pTempVertexBuffer->Map();
		memcpy(pMapped, vertices.data(), vertex_buffer_size);
		pTempVertexBuffer->Unmap();

		pMapped = pTempIndexBuffer->Map();
		memcpy(pMapped, indices.data(), index_buffer_size);
		pTempIndexBuffer->Unmap();

		pMapped = pTempTransformBuffer->Map();
		memcpy(pMapped, transforms.data(), transform_buffer_size);
		pTempTransformBuffer->Unmap();

		s_pComputeCommandAllocators[0]->Reset();
		s_pComputeCommandLists[0]->Begin(nullptr);

		BuildBottomLevelAccelerationStructureDesc buildDesc = { };
		buildDesc.pAccelerationStructure	= s_pBLAS;
		buildDesc.Flags						= 0;
		buildDesc.FirstVertexIndex			= 0;
		buildDesc.VertexStride				= sizeof(Vertex);
		buildDesc.IndexBufferByteOffset		= 0;
		buildDesc.TriangleCount				= 1;
		buildDesc.Update					= false;
		buildDesc.pVertexBuffer				= pTempVertexBuffer;
		buildDesc.pIndexBuffer				= pTempIndexBuffer;
		buildDesc.pTransformBuffer			= nullptr; //pTempTransformBuffer;
		buildDesc.TransformByteOffset		= 0;

		s_pComputeCommandLists[0]->BuildBottomLevelAccelerationStructure(&buildDesc);

		s_pComputeCommandLists[0]->End();

		ICommandList* pCommandList = s_pComputeCommandLists[0];
		RenderSystem::GetComputeQueue()->ExecuteCommandLists(&pCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, nullptr, 0, nullptr, 0);
		RenderSystem::GetComputeQueue()->Flush();
	}

	void RayTracingTestVK::CreateTLAS()
	{
		const GraphicsDeviceVK* pGraphicsDeviceVk = reinterpret_cast<const GraphicsDeviceVK*>(RenderSystem::GetDevice());

		AccelerationStructureDesc accelerationStructureDesc = {};
		accelerationStructureDesc.pName				= "TLAS";
		accelerationStructureDesc.Type				= EAccelerationStructureType::ACCELERATION_STRUCTURE_TOP;
		accelerationStructureDesc.Flags				= 0;
		accelerationStructureDesc.InstanceCount		= 1;
		accelerationStructureDesc.MaxTriangleCount	= 0;
		accelerationStructureDesc.MaxVertexCount	= 0;
		accelerationStructureDesc.AllowsTransform	= false;

		s_pTLAS = reinterpret_cast<AccelerationStructureVK*>(pGraphicsDeviceVk->CreateAccelerationStructure(&accelerationStructureDesc, nullptr));

		//VALIDATE(pBuildDesc->pInstanceBuffer != nullptr);

		VkResult result;

		VkTransformMatrixKHR transform_matrix = 
		{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f 
		};

		Instance instance{};
		memcpy(&instance.Transform, &transform_matrix, sizeof(instance.Transform));

		instance.MeshMaterialIndex				= 0;
		instance.Mask							= 0xFF;
		instance.SBTRecordOffset				= 0;
		instance.Flags							= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instance.AccelerationStructureAddress	= s_pBLAS->GetDeviceAdress();

		std::vector<Instance> instances(1, instance);

		BufferDesc instanceBufferDesc = {};
		instanceBufferDesc.pName		= "Temp Instance Buffer";
		instanceBufferDesc.MemoryType	= EMemoryType::MEMORY_CPU_VISIBLE;
		instanceBufferDesc.Flags		= FBufferFlags::BUFFER_FLAG_NONE;
		instanceBufferDesc.SizeInBytes	= instances.size() * sizeof(VkAccelerationStructureInstanceKHR);

		BufferVK* pTempInstanceBuffer = reinterpret_cast<BufferVK*>(pGraphicsDeviceVk->CreateBuffer(&instanceBufferDesc, nullptr));

		void* pMapped;

		pMapped = pTempInstanceBuffer->Map();
		memcpy(pMapped, instances.data(), instances.size() * sizeof(VkAccelerationStructureInstanceKHR));
		pTempInstanceBuffer->Unmap();

		s_pComputeCommandAllocators[0]->Reset();
		s_pComputeCommandLists[0]->Begin(nullptr);

		BuildTopLevelAccelerationStructureDesc buildDesc = { };
		buildDesc.pAccelerationStructure	= s_pTLAS;
		buildDesc.Flags						= 0;
		buildDesc.pInstanceBuffer			= pTempInstanceBuffer;
		buildDesc.InstanceCount				= 1;
		buildDesc.Update					= false;

		s_pComputeCommandLists[0]->BuildTopLevelAccelerationStructure(&buildDesc);

		s_pComputeCommandLists[0]->End();

		ICommandList* pCommandList = s_pComputeCommandLists[0];
		RenderSystem::GetComputeQueue()->ExecuteCommandLists(&pCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, nullptr, 0, nullptr, 0);
		RenderSystem::GetComputeQueue()->Flush();
	}

	void RayTracingTestVK::SetTLASFromScene(const Scene* pScene)
	{
		s_pSceneTLAS = reinterpret_cast<const AccelerationStructureVK*>(pScene->GetTLAS());
	}

	void RayTracingTestVK::GetAndIncrementFence(IFence** ppFence, uint64* pSignalValue)
	{
		(*pSignalValue)	= s_SignalValue++;
		(*ppFence)		= s_pFence;
	}

	void RayTracingTestVK::Render(uint64 modFrameIndex, uint32 backBufferIndex)
	{
		const GraphicsDeviceVK* pGraphicsDeviceVk = reinterpret_cast<const GraphicsDeviceVK*>(RenderSystem::GetDevice());

		s_pGraphicsPreCommandAllocators[modFrameIndex]->Reset();
		s_pGraphicsPostCommandAllocators[modFrameIndex]->Reset();
		s_pComputeCommandAllocators[modFrameIndex]->Reset();

		ICommandList* pGraphicsPreCommandList	= s_pGraphicsPreCommandLists[modFrameIndex];
		ICommandList* pComputeCommandList		= s_pComputeCommandLists[modFrameIndex];
		ICommandList* pGraphicsPostCommandList	= s_pGraphicsPostCommandLists[modFrameIndex];

		PipelineTextureBarrierDesc fromGraphicsToComputeBarrier = {};
		fromGraphicsToComputeBarrier.pTexture				= s_ppTextureViews[backBufferIndex]->GetDesc().pTexture;
		fromGraphicsToComputeBarrier.StateBefore			= ETextureState::TEXTURE_STATE_DONT_CARE;
		fromGraphicsToComputeBarrier.StateAfter				= ETextureState::TEXTURE_STATE_GENERAL;
		fromGraphicsToComputeBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
		fromGraphicsToComputeBarrier.QueueAfter				= ECommandQueueType::COMMAND_QUEUE_COMPUTE;
		fromGraphicsToComputeBarrier.SrcMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
		fromGraphicsToComputeBarrier.DstMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		//fromGraphicsToComputeBarrier.TextureFlags			= FTextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS;
		fromGraphicsToComputeBarrier.Miplevel				= 0;
		fromGraphicsToComputeBarrier.MiplevelCount			= 1;
		fromGraphicsToComputeBarrier.ArrayIndex				= 0;
		fromGraphicsToComputeBarrier.ArrayCount				= 1;

		PipelineTextureBarrierDesc fromComputeToGraphicsBarrier = {};
		fromComputeToGraphicsBarrier.pTexture				= s_ppTextureViews[backBufferIndex]->GetDesc().pTexture;
		fromComputeToGraphicsBarrier.StateBefore			= ETextureState::TEXTURE_STATE_GENERAL;
		fromComputeToGraphicsBarrier.StateAfter				= ETextureState::TEXTURE_STATE_PRESENT;
		fromComputeToGraphicsBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_COMPUTE;
		fromComputeToGraphicsBarrier.QueueAfter				= ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
		fromComputeToGraphicsBarrier.SrcMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		fromComputeToGraphicsBarrier.DstMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
		//fromComputeToGraphicsBarrier.TextureFlags			= FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		fromComputeToGraphicsBarrier.Miplevel				= 0;
		fromComputeToGraphicsBarrier.MiplevelCount			= 1;
		fromComputeToGraphicsBarrier.ArrayIndex				= 0;
		fromComputeToGraphicsBarrier.ArrayCount				= 1;

		pGraphicsPreCommandList->Begin(nullptr);
		pGraphicsPreCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER, &fromGraphicsToComputeBarrier, 1);
		pGraphicsPreCommandList->End();
		RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&pGraphicsPreCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, s_pFence, s_SignalValue - 1, s_pFence, s_SignalValue);
		s_SignalValue++;

		pComputeCommandList->Begin(nullptr);
		pComputeCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER, &fromGraphicsToComputeBarrier, 1);

		IPipelineState* pPipeline = PipelineStateManager::GetPipelineState(s_PipelineStateID);

		pComputeCommandList->BindRayTracingPipeline(pPipeline);
		pComputeCommandList->BindDescriptorSetRayTracing(s_pDescriptorSets[modFrameIndex], s_pPipelineLayout, 0);
		pComputeCommandList->TraceRays(1920, 1080, 1);

		pComputeCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, &fromComputeToGraphicsBarrier, 1);
		pComputeCommandList->End();
		RenderSystem::GetComputeQueue()->ExecuteCommandLists(&pComputeCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, s_pFence, s_SignalValue - 1, s_pFence, s_SignalValue);
		s_SignalValue++;

		pGraphicsPostCommandList->Begin(nullptr);
		pGraphicsPostCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, &fromComputeToGraphicsBarrier, 1);
		pGraphicsPostCommandList->End();
		RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&pGraphicsPostCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, s_pFence, s_SignalValue - 1, s_pFence, s_SignalValue);
		s_SignalValue++;
	}

	void RayTracingTestVK::Debug(IAccelerationStructure* pBLAS, IAccelerationStructure* pTLAS)
	{
		/*const GraphicsDeviceVK* pGraphicsDeviceVk = reinterpret_cast<const GraphicsDeviceVK*>(RenderSystem::GetDevice());
		AccelerationStructureVK* pBLASVk = reinterpret_cast<AccelerationStructureVK*>(pBLAS);
		AccelerationStructureVK* pTLASVk = reinterpret_cast<AccelerationStructureVK*>(pTLAS);

		VkResult result;

		uint64 serializedTLAS[10];
		uint64 serializedBLAS[10];

		memset(serializedTLAS, 0, sizeof(serializedTLAS));
		memset(serializedBLAS, 0, sizeof(serializedBLAS));

		{
			byte* pMapped;

			byte driverUUID[VK_UUID_SIZE];
			byte compatibility[VK_UUID_SIZE];
			uint64 size;
			uint64 deserializedSize;
			uint64 numASHandles;

			pMapped = reinterpret_cast<byte*>(s_pTLASSerializedBuffer->Map());

			memcpy(&driverUUID,			pMapped,														VK_UUID_SIZE);
			memcpy(&compatibility,		pMapped								+		VK_UUID_SIZE,		VK_UUID_SIZE);
			memcpy(&size,				pMapped								+ 2 *	VK_UUID_SIZE,		sizeof(uint64));
			memcpy(&deserializedSize,	pMapped		+		sizeof(uint64)	+ 2 *	VK_UUID_SIZE,		sizeof(uint64));
			memcpy(&numASHandles,		pMapped		+ 2 *	sizeof(uint64)	+ 2 *	VK_UUID_SIZE,		sizeof(uint64));
			memcpy(serializedTLAS,		pMapped		+ 3 *	sizeof(uint64)	+ 2 *	VK_UUID_SIZE,		numASHandles * sizeof(uint64));

			LOG_MESSAGE("TLAS Size: %u", size);
			LOG_MESSAGE("TLAS Deserialized Size: %u", deserializedSize);
			LOG_MESSAGE("TLAS Num AS Handles: %u", numASHandles);

			s_pTLASSerializedBuffer->Unmap();
		}

		{
			byte* pMapped;

			byte driverUUID[VK_UUID_SIZE];
			byte compatibility[VK_UUID_SIZE];
			uint64 size;
			uint64 deserializedSize;
			uint64 numASHandles;

			pMapped = reinterpret_cast<byte*>(s_pBLASSerializedBuffer->Map());

			memcpy(&driverUUID,			pMapped,														VK_UUID_SIZE);
			memcpy(&compatibility,		pMapped								+		VK_UUID_SIZE,		VK_UUID_SIZE);
			memcpy(&size,				pMapped								+ 2 *	VK_UUID_SIZE,		sizeof(uint64));
			memcpy(&deserializedSize,	pMapped		+		sizeof(uint64)	+ 2 *	VK_UUID_SIZE,		sizeof(uint64));
			memcpy(&numASHandles,		pMapped		+ 2 *	sizeof(uint64)	+ 2 *	VK_UUID_SIZE,		sizeof(uint64));
			memcpy(serializedBLAS,		pMapped		+ 3 *	sizeof(uint64)	+ 2 *	VK_UUID_SIZE,		numASHandles * sizeof(uint64));

			LOG_MESSAGE("BLAS Size: %u", size);
			LOG_MESSAGE("BLAS Deserialized Size: %u", deserializedSize);
			LOG_MESSAGE("BLAS Num AS Handles: %u", numASHandles);

			s_pBLASSerializedBuffer->Unmap();
		}

		int fim = 0;*/
	}
}