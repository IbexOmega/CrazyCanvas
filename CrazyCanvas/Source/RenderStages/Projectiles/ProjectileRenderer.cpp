#include "RenderStages/Projectiles/ProjectileRenderer.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Window.h"

#include "ECS/Components/Player/ProjectileComponent.h"
#include "Game/ECS/Components/Rendering/RayTracedComponent.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/GameConsole.h"

#include "Math/Random.h"

#include "Rendering/Core/API/Buffer.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/Sampler.h"
#include "Rendering/Core/API/Shader.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/EntityMaskManager.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"

#include "Resources/ResourceCatalog.h"
#include "Resources/ResourceManager.h"
#include "Teams/TeamHelper.h"

using namespace LambdaEngine;

ProjectileRenderer::ProjectileRenderer(GraphicsDevice* pGraphicsDevice)
	:	m_pGraphicsDevice(pGraphicsDevice)
{}

ProjectileRenderer::~ProjectileRenderer()
{
	ResourceManager::UnloadShader(m_DensityShaderGUID);
	ResourceManager::UnloadShader(m_GradientShaderGUID);
	ResourceManager::UnloadShader(m_MeshGenShaderGUID);
}

bool ProjectileRenderer::Init()
{
	CreatePipelineLayout();

	if (!CreateShaders())
	{
		LOG_ERROR("Failed to create marching cubes shader");
		return false;
	}

	return true;
}

bool ProjectileRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
{
	if (!CreateCommandLists(pPreInitDesc))
	{
		LOG_ERROR("Failed to create compute command lists");
		return false;
	}

	if (!CreatePipelines())
	{
		LOG_ERROR("Failed to create pipeline");
		return false;
	}

	if (!CreateDescriptorHeap())
	{
		LOG_ERROR("Failed to create descriptor heap");
		return false;
	}

	return true;
}

bool ProjectileRenderer::RenderGraphPostInit()
{
	CreateMarchingCubesGrids();
	SubscribeToProjectiles();

	return true;
}

void ProjectileRenderer::Update(LambdaEngine::Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
{
	UNREFERENCED_VARIABLE(modFrameIndex);
	UNREFERENCED_VARIABLE(backBufferIndex);

	const float32 dt = (float32)delta.AsSeconds();
	const float32 angularVelocity = glm::two_pi<float32>() * dt;

	for (MarchingCubesGrid& marchingCubesGrid : m_MarchingCubesGrids)
	{
		for (uint32 sphereIdx = 0; sphereIdx < SPHERES_PER_GRID; sphereIdx++)
		{
			glm::vec4& positionRadius = marchingCubesGrid.GPUData.SpherePositionsRadii[sphereIdx];
			const glm::vec3 position = glm::vec3(positionRadius) - glm::vec3(0.5f);
			const float32 distToCenter = glm::length(position);

			const glm::quat rotation = glm::angleAxis(angularVelocity, marchingCubesGrid.SphereRotationDirections[sphereIdx]);
			positionRadius = glm::vec4(glm::rotate(rotation, position) + glm::vec3(0.5f), positionRadius.w);
		}
	}
}

void ProjectileRenderer::UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)
{
	UNREFERENCED_VARIABLE(resourceName);

	// There shouldn't be more draw args than marching cubes grids as each draw arg should be assigned to one grid
	ASSERT(count <= UNIQUE_METABALLS_COUNT);
	if (m_InitializedGridCount == UNIQUE_METABALLS_COUNT)
	{
		return;
	}

	/*	An entity was assigned to a grid in ProjectileRenderer::OnProjectileAdded, find out which grid it was by
		finding the entity in DrawArg::EntityIDs. */
	for (uint32 gridNr = 0; gridNr < m_MarchingCubesGrids.size(); gridNr++)
	{
		MarchingCubesGrid& marchingCubesGrid = m_MarchingCubesGrids[gridNr];
		for (uint32 drawArgIdx = 0; drawArgIdx < count; drawArgIdx++)
		{
			const DrawArg& drawArg = pDrawArgs[drawArgIdx];
			if (std::any_of(drawArg.EntityIDs.begin(), drawArg.EntityIDs.end(),
				[&](Entity entity) { return marchingCubesGrid.InstancedEntities.contains(entity); }))
			{
				CreateRenderResources(gridNr, drawArg);
				m_InitializedGridCount++;
				return;
			}
		}
	}

	LOG_ERROR("Failed to link a draw arg to a marching cubes grid");
}

void ProjectileRenderer::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage, bool sleeping)
{
	UNREFERENCED_VARIABLE(backBufferIndex);
	UNREFERENCED_VARIABLE(ppFirstExecutionStage);
	UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);

	if (!sleeping && !m_ProjectileEntities.Empty())
	{
		RenderSystem& renderSystem = RenderSystem::GetInstance();
		ECSCore* pECS = ECSCore::GetInstance();
		const ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		const ComponentArray<ScaleComponent>* pScaleComponents = pECS->GetComponentArray<ScaleComponent>();
		const ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();

		const glm::bvec3 rotationalAxes(true);
		for (MarchingCubesGrid& marchingCubesGrid : m_MarchingCubesGrids)
		{
			if (marchingCubesGrid.InstancedEntities.empty())
			{
				continue;
			}

			const Entity entity = *marchingCubesGrid.InstancedEntities.begin();
			const glm::mat4 transform = RenderSystem::CreateEntityTransform(
				pPositionComponents->GetConstData(entity),
				pRotationComponents->GetConstData(entity),
				pScaleComponents->GetConstData(entity),
				rotationalAxes
			);

			renderSystem.UpdateTransformData(entity, transform);

			constexpr const bool isAnimated = false;
			constexpr const bool forceUniqueResource = false;
			constexpr const bool manualResourceDeletion = true;
			renderSystem.RebuildBLAS(entity, marchingCubesGrid.MeshGUID, isAnimated, forceUniqueResource, manualResourceDeletion);
		}

		CommandList* pCommandList = m_ComputeCommandLists[modFrameIndex].Get();
		m_ComputeCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);

		pCommandList->BindComputePipeline(m_PipelineDensityGen.Get());

		const uint32 initialTriangleCount = 0;

		// Generate density data
		for (MarchingCubesGrid& marchingCubesGrid : m_MarchingCubesGrids)
		{
			if (marchingCubesGrid.InstancedEntities.empty())
			{
				continue;
			}

			pCommandList->BindDescriptorSetCompute(marchingCubesGrid.DescriptorSet.Get(), m_PipelineLayout.Get(), 0);

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
			if (marchingCubesGrid.InstancedEntities.empty())
			{
				continue;
			}

			pCommandList->BindDescriptorSetCompute(marchingCubesGrid.DescriptorSet.Get(), m_PipelineLayout.Get(), 0);

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
			if (marchingCubesGrid.InstancedEntities.empty())
			{
				continue;
			}

			pCommandList->BindDescriptorSetCompute(marchingCubesGrid.DescriptorSet.Get(), m_PipelineLayout.Get(), 0);

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

float32 ProjectileRenderer::GetSpheresMaxDistToCenter(const MarchingCubesGrid& marchingCubesGrid)
{
	/*	Derived from the equation: Density = 1 and Density = R^2 / D^2, where R = sphere radius and
		D = distance to grid border. (See density function in compute shader) */
	float32 radiiSquaredSum = 0.0f;
	for (const glm::vec4& positionRadius : marchingCubesGrid.GPUData.SpherePositionsRadii)
	{
		radiiSquaredSum += positionRadius.w * positionRadius.w;
	}

	return 0.5f - std::sqrtf(radiiSquaredSum) - 1.1f / marchingCubesGrid.GPUData.GridWidth;
}

void ProjectileRenderer::RandomizeSpheres(MarchingCubesGrid& marchingCubesGrid)
{
	/* Each grid has a set of small spheres a larger spheres. The smaller spheres are placed at the grid's borde. */
	constexpr const uint32 largeSpheresCount = SPHERES_PER_GRID / 2;
	constexpr const uint32 smallSpheresCount = largeSpheresCount + 1;

	// Randomize radii
	constexpr const float32 minRadius = 0.07f;
	constexpr const float32 maxRadius = 0.10f;

	for (uint32 sphereIdx = 0; sphereIdx < SPHERES_PER_GRID; sphereIdx++)
	{
		marchingCubesGrid.GPUData.SpherePositionsRadii[sphereIdx].w = sphereIdx < smallSpheresCount ?
			minRadius : Random::Float32(minRadius, maxRadius);
	}

	// Randomize positions and rotations
	const float32 maxDistanceToCenter = GetSpheresMaxDistToCenter(marchingCubesGrid);
	constexpr const float32 speed = 0.4f;

	for (uint32 sphereIdx = 0; sphereIdx < SPHERES_PER_GRID; sphereIdx++)
	{
		// Position
		glm::vec4& positionRadius = marchingCubesGrid.GPUData.SpherePositionsRadii[sphereIdx];

		const glm::vec3 randPosition = { Random::Float32(), Random::Float32(), Random::Float32() };
		const float32 distanceToCenter = sphereIdx < smallSpheresCount ?
			maxDistanceToCenter : Random::Float32(maxDistanceToCenter / 2.0f, maxDistanceToCenter);

		positionRadius = glm::vec4(glm::vec3(0.5f) + glm::normalize(randPosition) * distanceToCenter, positionRadius.w);

		// Find a rotation vector that is perpendicular to the position vector
		const glm::vec3 positionDir = glm::normalize(glm::vec3(positionRadius) - glm::vec3(0.5f));
		marchingCubesGrid.SphereRotationDirections[sphereIdx] = RandomizePerpendicularVector(positionDir);
	}
}

glm::vec3 ProjectileRenderer::RandomizePerpendicularVector(const glm::vec3& referenceVec)
{
	// Find a non-parallel vector with which to perform cross product with the reference vector
	glm::vec3 nonParallelVector = g_DefaultRight;
	if (glm::dot(nonParallelVector, referenceVec) > 0.98f)
	{
		nonParallelVector = g_DefaultUp;
	}

	const glm::vec3 perpVector = glm::normalize(glm::cross(nonParallelVector, referenceVec));

	// A perpendicular vector has been found. Now rotate it about the reference vector using a random angle
	const float32 randRotAngle = Random::Float32(0.0f, glm::two_pi<float32>());

	const glm::quat rotQuat = glm::angleAxis(randRotAngle, referenceVec);
	return glm::rotate(rotQuat, perpVector);
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
	return m_DensityShaderGUID != GUID_NONE && m_GradientShaderGUID != GUID_NONE && m_MeshGenShaderGUID != GUID_NONE;
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

void ProjectileRenderer::CreateMarchingCubesGrids()
{
	/*	'Load' the first mesh to generate meshlets for it. Then duplicate the same mesh for the other instances.
		The duplication is only done in GPU memory. I.e. it is assumed that it is safe to have multiple GUIDs pointing
		at the same mesh, since it won't be edited on the CPU. */
	constexpr const uint32 maxTrianglesPerCell = 5;
	constexpr const uint32 verticesPerTriangle = 3;

	constexpr const uint32 cellGridWidth = GRID_WIDTH - 3;
	constexpr const uint32 cellCount = cellGridWidth * cellGridWidth * cellGridWidth;
	constexpr const uint32 vertexCount = cellCount * maxTrianglesPerCell * verticesPerTriangle;

	std::unique_ptr<Vertex> vertices(new Vertex[vertexCount]);
	memset(vertices.get(), 0, sizeof(Vertex) * vertexCount);

	uint32 indices[vertexCount];
	for (uint32 i = 0; i < vertexCount; i++)
	{
		indices[i] = vertexCount - 1 - i;
	}

	constexpr const bool useMeshletCache = true;
	GUID_Lambda meshGUID = ResourceManager::LoadMeshFromMemory("Marching Cubes Mesh 0", vertices.get(), vertexCount, indices, vertexCount, useMeshletCache);
	Mesh* pOriginalMesh = ResourceManager::GetMesh(meshGUID);

	for (uint32 gridNr = 0; gridNr < m_MarchingCubesGrids.size(); gridNr++)
	{
		const String meshName = "Marching Cubes Mesh " + std::to_string(gridNr);
		if (gridNr != 0)
		{
			meshGUID = ResourceManager::RegisterMesh(meshName, pOriginalMesh);
		}

		m_MarchingCubesGrids[gridNr] =
		{
			.GPUData =
			{
				.GridWidth = GRID_WIDTH
			},
			.MeshGUID = meshGUID
		};
	}
}

void ProjectileRenderer::CreateRenderResources(uint32 gridNr, const LambdaEngine::DrawArg& drawArg)
{
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

	MarchingCubesGrid& marchingCubesGrid = m_MarchingCubesGrids[gridNr];
	marchingCubesGrid.DensityBuffer = m_pGraphicsDevice->CreateBuffer(&densityBufferDesc);
	marchingCubesGrid.GradientBuffer = m_pGraphicsDevice->CreateBuffer(&gradientBufferDesc);
	marchingCubesGrid.DescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Marching Cubes Descriptor Set " + std::to_string(gridNr), m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());

	constexpr const uint32 maxTrianglesPerCell = 5;
	constexpr const uint32 verticesPerTriangle = 3;

	constexpr const uint32 cellGridWidth = GRID_WIDTH - 3;
	constexpr const uint32 cellCount = cellGridWidth * cellGridWidth * cellGridWidth;
	constexpr const uint32 vertexCount = cellCount * maxTrianglesPerCell * verticesPerTriangle;
	constexpr const uint64 vertexBufferSize = vertexCount * sizeof(Vertex);

	// Write descriptors
	constexpr const uint32 descriptorCount = 3;
	const Buffer* ppDescriptorSetBuffers[descriptorCount] = { marchingCubesGrid.DensityBuffer.Get(), marchingCubesGrid.GradientBuffer.Get(), drawArg.pVertexBuffer };
	const uint64 pOffsets[descriptorCount] = { 0, 0, 0 };
	const uint64 pSizes[descriptorCount] = { densityBufferDesc.SizeInBytes, gradientBufferDesc.SizeInBytes, vertexBufferSize };
	marchingCubesGrid.DescriptorSet->WriteBufferDescriptors(ppDescriptorSetBuffers, pOffsets, pSizes, 0, descriptorCount, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
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
					{ NDA, ProjectileComponent::Type() },
					{ NDA, PositionComponent::Type() },
					{ NDA, RotationComponent::Type() },
					{ NDA, ScaleComponent::Type() }
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
	MarchingCubesGrid& marchingCubesGrid = m_MarchingCubesGrids[m_NextMarchingCubesGrid];
	if (marchingCubesGrid.InstancedEntities.empty())
	{
		/*	The grid was previously unused. This is a good opportunity to randomize its spheres so it will have a new
			look when it is visible again. */
		RandomizeSpheres(marchingCubesGrid);
	}

	marchingCubesGrid.InstancedEntities.insert(entity);

	// Fetch the material to use
	const ECSCore* pECS = ECSCore::GetInstance();
	const ProjectileComponent& projectileComponent = pECS->GetConstComponent<ProjectileComponent>(entity);

	GUID_Lambda projectileMaterialGUID = ResourceCatalog::PROJECTILE_WATER_MATERIAL;
	if (projectileComponent.AmmoType == EAmmoType::AMMO_TYPE_PAINT)
	{
		const TeamComponent& teamComponent = pECS->GetConstComponent<TeamComponent>(entity);
		projectileMaterialGUID = TeamHelper::GetTeamColorMaterialGUID(teamComponent.TeamIndex);
	}

	const glm::mat4 transform = RenderSystem::CreateEntityTransform(entity, glm::bvec3(true));
	constexpr const bool isAnimated = false;
	constexpr const bool forceUniqueResource = false;
	constexpr const bool manualResourceDeletion = true;
	RenderSystem::GetInstance().AddRenderableEntity(
		entity,
		marchingCubesGrid.MeshGUID,
		projectileMaterialGUID,
		transform,
		isAnimated,
		forceUniqueResource,
		manualResourceDeletion);
}

void ProjectileRenderer::OnProjectileRemoval(LambdaEngine::Entity entity)
{
	MarchingCubesGrid& marchingCubesGrid = m_MarchingCubesGrids[m_NextMarchingCubesGrid];
	marchingCubesGrid.InstancedEntities.erase(entity);

	RenderSystem& renderSystem = RenderSystem::GetInstance();
	renderSystem.RemoveRenderableEntity(entity);
}
