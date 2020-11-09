#include "RenderStages/Projectiles/ProjectileRenderer.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderGraph.h"

#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/Sampler.h"
#include "Rendering/Core/API/Shader.h"
#include "Rendering/Core/API/Buffer.h"
#include "Rendering/EntityMaskManager.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "ECS/Components/Player/ProjectileComponent.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "Resources/ResourceManager.h"

#include "Game/GameConsole.h"

using namespace LambdaEngine;

ProjectileRenderer::ProjectileRenderer(GraphicsDevice* pGraphicsDevice)
	:	m_pGraphicsDevice(pGraphicsDevice)
{}

ProjectileRenderer::~ProjectileRenderer()
{
	ResourceManager::UnloadShader(m_DensityShaderGUID);
	ResourceManager::UnloadShader(m_MeshGenShaderGUID);

	for (MarchingCubesGrid& marchingCubesGrid : m_MarchingCubesGrids)
	{
		DeleteMarchingCubesGrid(marchingCubesGrid);
	}
}

bool ProjectileRenderer::Init()
{
	CreatePipelineLayout();

	if (!CreateShaders())
	{
		LOG_ERROR("[ProjectileRenderer]: Failed to create marching cubes shader");
		return false;
	}

	if (!CreateCommonMesh())
	{
		LOG_ERROR("[ProjectileRenderer]: Failed to create common mesh");
		return false;
	}

	return true;
}

bool ProjectileRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
{
	if (!CreateCommandLists(pPreInitDesc))
	{
		LOG_ERROR("[ProjectileRenderer]: Failed to create compute command lists");
		return false;
	}

	if (!CreatePipeline())
	{
		LOG_ERROR("[ProjectileRenderer]: Failed to create pipeline");
		return false;
	}

	if (!CreateDescriptorHeap())
	{
		LOG_ERROR("[ProjectileRenderer]: Failed to create descriptor heap");
		return false;
	}

	SubscribeToProjectiles();
	// OnProjectileCreated(0);

	return true;
}

void ProjectileRenderer::UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)
{
	UNREFERENCED_VARIABLE(resourceName);

	for (uint32 drawArgIdx = 0; drawArgIdx < count; drawArgIdx++)
	{
		const DrawArg& drawArg = pDrawArgs[drawArgIdx];

		// Projectiles should not be instanced as each one should have its own vertex buffer that can be edited.
		ASSERT(drawArg.EntityIDs.GetSize() == 1);

		const Entity entity = drawArg.EntityIDs.GetFront();

		const BufferDesc triangleCountBufferDesc =
		{
			.DebugName		= "Projectile Triangle Count Buffer",
			.MemoryType 	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE,
			.Flags			= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
			.SizeInBytes	= sizeof(uint32)
		};

		const MarchingCubesGrid marchingCubesGrid =
		{
			.GPUData =
			{
				.GridWidth = GRID_WIDTH,
				.SphereCount = 1,
				.SpherePositions =
				{
					{ 0.5f, 0.5f, 0.5f }
				}
			},
			.SphereVelocities =
			{
				{ 0.0f, 0.0f, 0.0f }
			},
			.pTriangleCountBuffer = m_pGraphicsDevice->CreateBuffer(&triangleCountBufferDesc),
			.pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Marching Cubes Descriptor Set", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get())
		};

		const uint64 gridCorners = marchingCubesGrid.GPUData.GridWidth * marchingCubesGrid.GPUData.GridWidth * marchingCubesGrid.GPUData.GridWidth;
		const uint64 vertexBufferSize = gridCorners * sizeof(Vertex);

		// Write descriptors
		const Buffer* ppDescriptorSetBuffers[2] = { drawArg.pVertexBuffer, marchingCubesGrid.pTriangleCountBuffer };
		const uint64 pOffsets[2] = { 0, 0 };
		const uint64 pSizes[2] = { vertexBufferSize, triangleCountBufferDesc.SizeInBytes };
		marchingCubesGrid.pDescriptorSet->WriteBufferDescriptors(ppDescriptorSetBuffers, pOffsets, pSizes, 0, 2, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);

		m_MarchingCubesGrids.PushBack(marchingCubesGrid, entity);
	}
}

void ProjectileRenderer::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage, bool sleeping)
{
	UNREFERENCED_VARIABLE(backBufferIndex);
	UNREFERENCED_VARIABLE(ppFirstExecutionStage);
	UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);
	UNREFERENCED_VARIABLE(sleeping);

	if (!m_MarchingCubesGrids.Empty())
	{
		CommandList* pCommandList = m_ppComputeCommandLists[modFrameIndex];
		// const SecondaryCommandListBeginDesc beginDesc = {};
		m_ppComputeCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);

		pCommandList->BindComputePipeline(m_Pipeline.Get());

		const uint32 initialTriangleCount = 0;

		for (MarchingCubesGrid& marchingCubesGrid : m_MarchingCubesGrids)
		{
			pCommandList->BindDescriptorSetCompute(marchingCubesGrid.pDescriptorSet, m_PipelineLayout.Get(), 0);

			// The triangle count should be reset at the beginning of the dispatch
			void* pTriangleCountData = marchingCubesGrid.pTriangleCountBuffer->Map();
			memcpy(pTriangleCountData, &initialTriangleCount, sizeof(uint32));
			marchingCubesGrid.pTriangleCountBuffer->Unmap();

			// Figure out the minimum amount of work groups to dispatch one thread for each cell
			const uint32 gridWidth = marchingCubesGrid.GPUData.GridWidth;
			const uint32 cellCount = gridWidth * gridWidth * gridWidth;
			const uint32 workGroupCount = (uint32)std::ceilf((float32)cellCount / m_WorkGroupSize);

			pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, &marchingCubesGrid.GPUData, sizeof(GridConstantRange), 0);
			pCommandList->Dispatch(workGroupCount, 1, 1);
		}

		pCommandList->End();
	}
}

void ProjectileRenderer::CreatePipelineLayout()
{
	const ConstantRangeDesc constantRangeDesc	=
	{
		.ShaderStageFlags	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER,
		.SizeInBytes		= sizeof(GridConstantRange),
		.OffsetInBytes		= 0
	};

	// Triangle buffer
	const DescriptorBindingDesc triangleBufferDesc =
	{
		.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER,
		.DescriptorCount	= 1,
		.Binding			= 0,
		.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER
	};

	// Triangle counter buffer
	const DescriptorBindingDesc triangleCounterDesc =
	{
		.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER,
		.DescriptorCount	= 1,
		.Binding			= 1,
		.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER
	};

	const DescriptorSetLayoutDesc descriptorSetLayoutDesc =
	{
		.DescriptorBindings = { triangleBufferDesc, triangleCounterDesc }
	};

	const PipelineLayoutDesc pipelineLayoutDesc =
	{
		.DebugName = "Marching Cubes Pipeline Layout",
		.DescriptorSetLayouts = { descriptorSetLayoutDesc },
		.ConstantRanges = { constantRangeDesc }
	};

	m_PipelineLayout = m_pGraphicsDevice->CreatePipelineLayout(&pipelineLayoutDesc);
}

bool ProjectileRenderer::CreateShaders()
{
	m_DensityShaderGUID	= ResourceManager::LoadShaderFromFile("/Projectiles/MarchingCubesDensity.comp", FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);
	m_MeshGenShaderGUID	= ResourceManager::LoadShaderFromFile("/Projectiles/MarchingCubesMeshGen.comp", FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);
	return m_DensityShaderGUID != GUID_NONE && m_MeshGenShaderGUID != GUID_NONE;
}

bool ProjectileRenderer::CreateCommonMesh()
{
	constexpr const uint32 vertexCount = GRID_WIDTH * GRID_WIDTH * GRID_WIDTH;
	Vertex vertices[vertexCount];
	memset(vertices, 0, sizeof(Vertex) * vertexCount);

	uint32 indices[vertexCount];
	for (uint32 i = 0; i < vertexCount; i++)
	{
		indices[i] = i;
	}

	m_MarchingCubesMesh = ResourceManager::LoadMeshFromMemory("Marching Cubes Common Mesh", vertices, vertexCount, indices, vertexCount);
	return m_MarchingCubesMesh != GUID_NONE;
}

bool ProjectileRenderer::CreateDescriptorHeap()
{
	DescriptorHeapInfo descriptorCountDesc = {};
	descriptorCountDesc.SamplerDescriptorCount					= 0;
	descriptorCountDesc.TextureDescriptorCount					= 0;
	descriptorCountDesc.TextureCombinedSamplerDescriptorCount	= 0;
	descriptorCountDesc.ConstantBufferDescriptorCount			= 0;
	descriptorCountDesc.UnorderedAccessBufferDescriptorCount	= 2;
	descriptorCountDesc.UnorderedAccessTextureDescriptorCount	= 0;
	descriptorCountDesc.AccelerationStructureDescriptorCount	= 0;

	const DescriptorHeapDesc descriptorHeapDesc =
	{
		.DebugName			= "Projectile Renderer Descriptor Heap",
		.DescriptorSetCount	= 256,
		.DescriptorCount	= descriptorCountDesc
	};

	m_DescriptorHeap = m_pGraphicsDevice->CreateDescriptorHeap(&descriptorHeapDesc);
	return m_DescriptorHeap.Get();
}

bool ProjectileRenderer::CreateCommandLists(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
{
	const uint32 backBufferCount = pPreInitDesc->BackBufferCount;

	m_ppComputeCommandAllocators = DBG_NEW CommandAllocator * [backBufferCount];
	m_ppComputeCommandLists = DBG_NEW CommandList * [backBufferCount];

	for (uint32 b = 0; b < backBufferCount; b++)
	{
		m_ppComputeCommandAllocators[b] = m_pGraphicsDevice->CreateCommandAllocator("Projectile Renderer Compute Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);
		if (!m_ppComputeCommandAllocators[b])
		{
			return false;
		}

		const CommandListDesc commandListDesc =
		{
			.DebugName			= "Projectile Renderer Render Command List " + std::to_string(b),
			.CommandListType	= ECommandListType::COMMAND_LIST_TYPE_PRIMARY,
			.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT
		};

		m_ppComputeCommandLists[b] = m_pGraphicsDevice->CreateCommandList(m_ppComputeCommandAllocators[b], &commandListDesc);
		if (!m_ppComputeCommandLists[b])
		{
			return false;
		}
	}

	return true;
}

bool ProjectileRenderer::CreatePipeline()
{
	/*	Set the work group size. It could be larger (x * y * z), but smaller and more work groups allows for more
		concurrency, hence it is faster. */
	GraphicsDeviceFeatureDesc deviceFeatures;
	m_pGraphicsDevice->QueryDeviceFeatures(&deviceFeatures);
	m_WorkGroupSize = deviceFeatures.MaxComputeWorkGroupSize[0];

	const ComputePipelineStateDesc pipelineStateDesc =
	{
		.DebugName = "Marching Cubes Pipeline",
		.pPipelineLayout = m_PipelineLayout.Get(),
		.Shader =
		{
			.pShader = ResourceManager::GetShader(m_DensityShaderGUID),
			.ShaderConstants =
			{
				// local_size_x
				{
					.Integer = (int32)m_WorkGroupSize
				}
			}
		}
	};

	m_Pipeline = m_pGraphicsDevice->CreateComputePipelineState(&pipelineStateDesc);
	return m_Pipeline.Get();
}

void ProjectileRenderer::SubscribeToProjectiles()
{
	EntitySubscriberRegistration subscriberReg =
	{
		{
			{
				.pSubscriber = &m_ProjectileEntities,
				.ComponentAccesses =
				{
					{NDA, ProjectileComponent::Type()},
					{NDA, PositionComponent::Type()},
					{NDA, RotationComponent::Type()},
					{NDA, ScaleComponent::Type()}
				},
				.OnEntityAdded = std::bind_front(&ProjectileRenderer::OnProjectileCreated, this),
				.OnEntityRemoval = std::bind_front(&ProjectileRenderer::OnProjectileRemoval, this)
			},
		}
	};

	SubscribeToEntities(subscriberReg);
}

void ProjectileRenderer::OnProjectileCreated(LambdaEngine::Entity entity)
{
	LOG_INFO("%u", entity);
	RenderSystem& renderSystem = RenderSystem::GetInstance();
	renderSystem.AddRenderableEntity(entity, m_MarchingCubesMesh, GUID_MATERIAL_DEFAULT, RenderSystem::CreateEntityTransform(entity, glm::bvec3(true)), false, true);
}

void ProjectileRenderer::OnProjectileRemoval(LambdaEngine::Entity entity)
{
	UNREFERENCED_VARIABLE(entity);
}

void ProjectileRenderer::DeleteMarchingCubesGrid(MarchingCubesGrid& marchingCubesGrid)
{
	SAFEDELETE(marchingCubesGrid.pTriangleCountBuffer);
	SAFEDELETE(marchingCubesGrid.pDescriptorSet);
}
