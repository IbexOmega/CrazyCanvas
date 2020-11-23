#pragma once

#include "Containers/IDVector.h"
#include "ECS/EntitySubscriber.h"
#include "Rendering/Core/API/PipelineContext.h"
#include "Rendering/CustomRenderer.h"
#include "Rendering/RenderGraph.h"

#define GRID_WIDTH 18
#define SPHERES_PER_GRID 3

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
		static const LambdaEngine::String name = LambdaEngine::RENDER_GRAPH_PROJECTILES_NAME;
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
	bool CreateCommonMesh();
	bool CreateDescriptorHeap();
	bool CreateCommandLists(const LambdaEngine::CustomRendererRenderGraphInitDesc* pPreInitDesc);
	bool CreatePipelines();
	// Subscribes to projectile entities
	void SubscribeToProjectiles();

	void OnProjectileCreated(LambdaEngine::Entity entity);
	void OnProjectileRemoval(LambdaEngine::Entity entity);

private:
	LambdaEngine::IDDVector<MarchingCubesGrid> m_MarchingCubesGrids;
	LambdaEngine::IDVector m_ProjectileEntities;

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

	/*	All projectiles share the same mesh to avoid having to recreate an empty vertex buffer each time a
		projectile is created. */
	GUID_Lambda m_MarchingCubesMesh = GUID_NONE;

	// All marching cubes share the same mesh, which has a constant size. This is the size in bytes.
	uint64 m_VertexBufferSize = 0;
};
