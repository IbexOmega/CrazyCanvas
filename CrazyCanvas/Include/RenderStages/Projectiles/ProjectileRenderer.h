#pragma once

#include "Containers/IDVector.h"
#include "ECS/EntitySubscriber.h"
#include "Rendering/Core/API/PipelineContext.h"
#include "Rendering/CustomRenderer.h"
#include "Rendering/RenderGraph.h"

#include <array>
#include <unordered_set>

#define GRID_WIDTH 18
#define SPHERES_PER_GRID 7

#define UNIQUE_METABALLS_COUNT 5

namespace LambdaEngine
{
	class Buffer;
	class CommandAllocator;
	class CommandList;
	class DescriptorHeap;
	class DescriptorSet;
	class GraphicsDevice;
	class PipelineLayout;
}

// GridConstantRange contains data that is sent to the compute shader
struct GridConstantRange
{
	// The radius is stored in the .w component
	glm::vec4 SpherePositionsRadii[SPHERES_PER_GRID];
	uint32 GridWidth;	// The grid is cubic, i.e. the corner count is GridWidth^3
};

struct MarchingCubesGrid
{
	GridConstantRange GPUData;
	glm::vec3 SphereRotationDirections[SPHERES_PER_GRID];
	std::unordered_set<LambdaEngine::Entity> InstancedEntities;
	GUID_Lambda MeshGUID;
	LambdaEngine::TSharedRef<LambdaEngine::Buffer> DensityBuffer;
	LambdaEngine::TSharedRef<LambdaEngine::Buffer> GradientBuffer;
	LambdaEngine::TSharedRef<LambdaEngine::DescriptorSet> DescriptorSet;
};

class ProjectileRenderer : public LambdaEngine::CustomRenderer, LambdaEngine::EntitySubscriber
{
public:
	ProjectileRenderer(LambdaEngine::GraphicsDevice* pGraphicsDevice);
	~ProjectileRenderer();

	bool Init() override final;
	bool RenderGraphInit(const LambdaEngine::CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;
	bool RenderGraphPostInit() override final;
	void Finalize();

	void Update(LambdaEngine::Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex) override final;
	void UpdateDrawArgsResource(const LambdaEngine::String& resourceName, const LambdaEngine::DrawArg* pDrawArgs, uint32 count) override final;
	void Render(
		uint32 modFrameIndex,
		uint32 backBufferIndex,
		LambdaEngine::CommandList** ppFirstExecutionStage,
		LambdaEngine::CommandList** ppSecondaryExecutionStage,
		bool sleeping) override final;

	FORCEINLINE LambdaEngine::FPipelineStageFlag GetFirstPipelineStage() const override final	{ return LambdaEngine::FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }
	FORCEINLINE LambdaEngine::FPipelineStageFlag GetLastPipelineStage() const override final	{ return LambdaEngine::FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }

	FORCEINLINE const LambdaEngine::String& GetName() const override final
	{
		static const LambdaEngine::String name = "RENDER_STAGE_PROJECTILES";
		return name;
	}

private:
	// GetSpheresMaxDistToCenter calculates the grid's spheres' maximum distance to the grid's center position.
	static float32 GetSpheresMaxDistToCenter(const MarchingCubesGrid& marchingCubesGrid);
	// RandomPositionRadius generates a random position and radius for a sphere
	static void RandomizeSpheres(MarchingCubesGrid& marchingCubesGrid);
	static glm::vec3 RandomizePerpendicularVector(const glm::vec3& referenceVec);

private:
	void CreatePipelineLayout();
	bool CreateShaders();
	bool CreateDescriptorHeap();
	bool CreateCommandLists(const LambdaEngine::CustomRendererRenderGraphInitDesc* pPreInitDesc);
	bool CreatePipelines();
	void CreateMarchingCubesGrids();

	// Create buffers and a descriptor set for a marching cubes grid
	void CreateRenderResources(uint32 gridNr, const LambdaEngine::DrawArg& drawArg);
	// Subscribes to projectile entities
	void SubscribeToProjectiles();

	void OnProjectileCreated(LambdaEngine::Entity entity);
	void OnProjectileRemoval(LambdaEngine::Entity entity);

private:
	std::array<MarchingCubesGrid, UNIQUE_METABALLS_COUNT> m_MarchingCubesGrids;
	LambdaEngine::IDVector m_ProjectileEntities;

	// Index to the marching cubes grid that will be assigned to the next projectile entity
	uint32 m_NextMarchingCubesGrid = 0;

	/*	When UpdateDrawArgsResource is called, each marching cubes grid is checked to see if its rendering resource
		have been initialized. If not, the draw arg's vertex buffer is used to create a descriptor set for the grid.
		This counter flags whether or not that has already been done for every grid. */
	uint32 m_InitializedGridCount = 0;

	const LambdaEngine::GraphicsDevice* m_pGraphicsDevice = nullptr;

	LambdaEngine::TSharedRef<LambdaEngine::PipelineLayout> m_PipelineLayout = nullptr;
	LambdaEngine::TSharedRef<LambdaEngine::PipelineState> m_PipelineDensityGen = nullptr;
	LambdaEngine::TSharedRef<LambdaEngine::PipelineState> m_PipelineGradientGen = nullptr;
	LambdaEngine::TSharedRef<LambdaEngine::PipelineState> m_PipelineMeshGen = nullptr;
	LambdaEngine::TSharedRef<LambdaEngine::DescriptorHeap> m_DescriptorHeap = nullptr;

	LambdaEngine::TArray<LambdaEngine::TSharedRef<LambdaEngine::CommandAllocator>> m_ComputeCommandAllocators;
	LambdaEngine::TArray<LambdaEngine::TSharedRef<LambdaEngine::CommandList>> m_ComputeCommandLists;

	GUID_Lambda m_DensityShaderGUID = GUID_NONE;
	GUID_Lambda m_GradientShaderGUID = GUID_NONE;
	GUID_Lambda m_MeshGenShaderGUID = GUID_NONE;
	uint32 m_WorkGroupSize = 0;

	// All marching cubes share the same mesh, which has a constant size. This is the size in bytes.
	uint64 m_VertexBufferSize = 0;
};
