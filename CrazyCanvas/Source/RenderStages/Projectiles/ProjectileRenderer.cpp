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

	if (!CreatePipelines())
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
		const uint64 gridCorners = GRID_WIDTH * GRID_WIDTH * GRID_WIDTH;

		const BufferDesc densityBufferDesc =
		{
			.DebugName		= "Projectile Density Buffer",
			.MemoryType 	= EMemoryType::MEMORY_TYPE_GPU,
			.Flags			= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
			.SizeInBytes	= gridCorners * sizeof(float32)
		};

		/*	Gradients can't be calculated for corners on the grid's border. Thus, gradients are calculated on the
			'inner grid'. */
		constexpr const uint64 innerGridWidth = GRID_WIDTH - 2;
		constexpr const uint64 innerGridCorners = innerGridWidth * innerGridWidth * innerGridWidth;

		const BufferDesc gradientBufferDesc =
		{
			.DebugName		= "Projectile Gradient Buffer",
			.MemoryType 	= EMemoryType::MEMORY_TYPE_GPU,
			.Flags			= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
			.SizeInBytes	= sizeof(glm::vec4) * innerGridCorners
		};

		const MarchingCubesGrid marchingCubesGrid =
		{
			.GPUData =
			{
				.SpherePositions =
				{
					{ 0.5f, 0.5f, 0.5f, 0.0f }
				},
				.GridWidth = GRID_WIDTH,
				.SphereCount = 1
			},
			.SphereVelocities =
			{
				{ 0.0f, 0.0f, 0.0f }
			},
			.pDensityBuffer = m_pGraphicsDevice->CreateBuffer(&densityBufferDesc),
			.pGradientBuffer = m_pGraphicsDevice->CreateBuffer(&gradientBufferDesc),
			.pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Marching Cubes Descriptor Set", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get())
		};

		constexpr const uint64 vertexBufferSize = gridCorners * sizeof(Vertex);

		// Write descriptors
		constexpr const uint32 descriptorCount = 3;
		const Buffer* ppDescriptorSetBuffers[descriptorCount] = { marchingCubesGrid.pDensityBuffer, marchingCubesGrid.pGradientBuffer, drawArg.pVertexBuffer };
		const uint64 pOffsets[descriptorCount] = { 0, 0, 0 };
		const uint64 pSizes[descriptorCount] = { densityBufferDesc.SizeInBytes, gradientBufferDesc.SizeInBytes, vertexBufferSize };
		marchingCubesGrid.pDescriptorSet->WriteBufferDescriptors(ppDescriptorSetBuffers, pOffsets, pSizes, 0, descriptorCount, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);

		drawArg.pVertexBuffer->SetName("Projectile Vertex Buffer");
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
		CommandList* pCommandList = m_ComputeCommandLists[modFrameIndex].Get();
		m_ComputeCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);

		pCommandList->BindComputePipeline(m_PipelineDensityGen.Get());

		const uint32 initialTriangleCount = 0;

		// Generate density data
		for (MarchingCubesGrid& marchingCubesGrid : m_MarchingCubesGrids)
		{
			pCommandList->BindDescriptorSetCompute(marchingCubesGrid.pDescriptorSet, m_PipelineLayout.Get(), 0);

			// Figure out the minimum amount of work groups to dispatch one thread for each cell
			const uint32 gridWidth = marchingCubesGrid.GPUData.GridWidth;
			const uint32 cornerCount = gridWidth * gridWidth * gridWidth;
			const uint32 workGroupCount = (uint32)std::ceilf((float32)cornerCount / m_WorkGroupSize);

			pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, &marchingCubesGrid.GPUData, sizeof(GridConstantRange), 0);
			pCommandList->Dispatch(workGroupCount, 1, 1);
		}

		// Generate gradients using the density data
		pCommandList->BindComputePipeline(m_PipelineGradientGen.Get());

		for (MarchingCubesGrid& marchingCubesGrid : m_MarchingCubesGrids)
		{
			pCommandList->BindDescriptorSetCompute(marchingCubesGrid.pDescriptorSet, m_PipelineLayout.Get(), 0);

			// Figure out the minimum amount of work groups to dispatch one thread for each cell
			const uint32 innerGridWidth = marchingCubesGrid.GPUData.GridWidth - 2;
			const uint32 cornerCount = innerGridWidth * innerGridWidth * innerGridWidth;
			const uint32 workGroupCount = (uint32)std::ceilf((float32)cornerCount / m_WorkGroupSize);

			pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, &marchingCubesGrid.GPUData, sizeof(GridConstantRange), 0);
			pCommandList->Dispatch(workGroupCount, 1, 1);
		}

		// Generate meshes using the density and gradients
		pCommandList->BindComputePipeline(m_PipelineMeshGen.Get());

		for (MarchingCubesGrid& marchingCubesGrid : m_MarchingCubesGrids)
		{
			pCommandList->BindDescriptorSetCompute(marchingCubesGrid.pDescriptorSet, m_PipelineLayout.Get(), 0);

			// Figure out the minimum amount of work groups to dispatch one thread for each cell
			const uint32 gridWidth = marchingCubesGrid.GPUData.GridWidth;
			const uint32 gridWidthInCells = gridWidth - 3; // Do not compute cells at the border of the grid
			const uint32 cellCount = gridWidthInCells * gridWidthInCells * gridWidthInCells;
			const uint32 workGroupCount = (uint32)std::ceilf((float32)cellCount / m_WorkGroupSize);

			pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, &marchingCubesGrid.GPUData, sizeof(GridConstantRange), 0);
			pCommandList->Dispatch(workGroupCount, 1, 1);
		}

		pCommandList->End();
		(*ppFirstExecutionStage) = pCommandList;
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

	const DescriptorBindingDesc densityBufferDesc =
	{
		.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER,
		.DescriptorCount	= 1,
		.Binding			= 0,
		.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER
	};

	const DescriptorBindingDesc gradientBufferDesc =
	{
		.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER,
		.DescriptorCount	= 1,
		.Binding			= 1,
		.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER
	};

	const DescriptorBindingDesc triangleBufferDesc =
	{
		.DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER,
		.DescriptorCount	= 1,
		.Binding			= 2,
		.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER
	};

	const DescriptorSetLayoutDesc descriptorSetLayoutDesc =
	{
		.DescriptorBindings = { densityBufferDesc, gradientBufferDesc, triangleBufferDesc }
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
	m_DensityShaderGUID		= ResourceManager::LoadShaderFromFile("/Projectiles/MarchingCubesDensity.comp", FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);
	m_GradientShaderGUID	= ResourceManager::LoadShaderFromFile("/Projectiles/MarchingCubesGradient.comp", FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);
	m_MeshGenShaderGUID		= ResourceManager::LoadShaderFromFile("/Projectiles/MarchingCubesMeshGen.comp", FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);
	return m_DensityShaderGUID != GUID_NONE && m_GradientShaderGUID && m_MeshGenShaderGUID != GUID_NONE;
}

bool ProjectileRenderer::CreateCommonMesh()
{
	constexpr const uint32 maxTrianglesPerCell = 5;
	constexpr const uint32 verticesPerTriangle = 3;

	constexpr const uint32 cellGridWidth = GRID_WIDTH - 3;
	constexpr const uint32 cellCount = cellGridWidth * cellGridWidth * cellGridWidth;
	constexpr const uint32 vertexCount = cellCount * maxTrianglesPerCell * verticesPerTriangle;

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

	m_ComputeCommandAllocators.Resize(backBufferCount);
	m_ComputeCommandLists.Resize(backBufferCount);

	for (uint32 b = 0; b < backBufferCount; b++)
	{
		m_ComputeCommandAllocators[b] = m_pGraphicsDevice->CreateCommandAllocator("Projectile Renderer Compute Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);
		if (!m_ComputeCommandAllocators[b])
		{
			return false;
		}

		const CommandListDesc commandListDesc =
		{
			.DebugName			= "Projectile Renderer Command List " + std::to_string(b),
			.CommandListType	= ECommandListType::COMMAND_LIST_TYPE_PRIMARY,
			.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT
		};

		m_ComputeCommandLists[b] = m_pGraphicsDevice->CreateCommandList(m_ComputeCommandAllocators[b].Get(), &commandListDesc);
		if (!m_ComputeCommandLists[b])
		{
			return false;
		}
	}

	return true;
}

bool ProjectileRenderer::CreatePipelines()
{
	GraphicsDeviceFeatureDesc deviceFeatures;
	m_pGraphicsDevice->QueryDeviceFeatures(&deviceFeatures);
	m_WorkGroupSize = deviceFeatures.MaxComputeWorkGroupSize[0];

	// Create density generation pipeline
	{
		const ComputePipelineStateDesc pipelineStateDesc =
		{
			.DebugName = "Marching Cubes Density Generation Pipeline",
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

		m_PipelineDensityGen = m_pGraphicsDevice->CreateComputePipelineState(&pipelineStateDesc);
	}

	// Create gradient generation pipeline
	{
		const ComputePipelineStateDesc pipelineStateDesc =
		{
			.DebugName = "Marching Cubes Gradient Generation Pipeline",
			.pPipelineLayout = m_PipelineLayout.Get(),
			.Shader =
			{
				.pShader = ResourceManager::GetShader(m_GradientShaderGUID),
				.ShaderConstants =
				{
					// local_size_x
					{
						.Integer = (int32)m_WorkGroupSize
					}
				}
			}
		};

		m_PipelineGradientGen = m_pGraphicsDevice->CreateComputePipelineState(&pipelineStateDesc);
	}

	// Create mesh generation pipeline
	{
		const ComputePipelineStateDesc pipelineStateDesc =
		{
			.DebugName = "Marching Cubes Mesh Generation Pipeline",
			.pPipelineLayout = m_PipelineLayout.Get(),
			.Shader =
			{
				.pShader = ResourceManager::GetShader(m_MeshGenShaderGUID),
				.ShaderConstants =
				{
					// local_size_x
					{
						.Integer = (int32)m_WorkGroupSize
					}
				}
			}
		};

		m_PipelineMeshGen = m_pGraphicsDevice->CreateComputePipelineState(&pipelineStateDesc);
	}

	return m_PipelineDensityGen.Get() && m_PipelineMeshGen.Get();
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
	MarchingCubesGrid& marchingCubesGrid = m_MarchingCubesGrids.IndexID(entity);
	SAFEDELETE(marchingCubesGrid.pDensityBuffer);
	SAFEDELETE(marchingCubesGrid.pGradientBuffer);
	SAFEDELETE(marchingCubesGrid.pDescriptorSet);

	m_MarchingCubesGrids.Pop(entity);
}

void ProjectileRenderer::DeleteMarchingCubesGrid(MarchingCubesGrid& marchingCubesGrid)
{
	SAFEDELETE(marchingCubesGrid.pDensityBuffer);
	SAFEDELETE(marchingCubesGrid.pGradientBuffer);
	SAFEDELETE(marchingCubesGrid.pDescriptorSet);
}
