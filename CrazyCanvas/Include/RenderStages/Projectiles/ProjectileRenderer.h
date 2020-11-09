#pragma once

#include "Containers/IDVector.h"
#include "ECS/EntitySubscriber.h"
#include "Rendering/Core/API/PipelineContext.h"
#include "Rendering/CustomRenderer.h"
#include "Rendering/RenderGraph.h"

#define GRID_WIDTH 10

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
	// The grid is uniform, i.e. the cell count is GridWidth^3
	uint32 GridWidth;
	uint32 SphereCount;
	glm::vec3 SpherePositions[10];
};

struct MarchingCubesGrid
{
	GridConstantRange GPUData;
	glm::vec3 SphereVelocities[10];
	// pTriangleCountBuffer contains a uint32 that is atomically added when a triangle is added to the mesh
	LambdaEngine::Buffer* pTriangleCountBuffer;
	LambdaEngine::DescriptorSet* pDescriptorSet;
};

class ProjectileRenderer : public LambdaEngine::CustomRenderer, LambdaEngine::EntitySubscriber
{
public:
	ProjectileRenderer(LambdaEngine::GraphicsDevice* pGraphicsDevice);
	~ProjectileRenderer();

	bool Init() override final;

	bool RenderGraphInit(const LambdaEngine::CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;
	void UpdateDrawArgsResource(const LambdaEngine::String& resourceName, const LambdaEngine::DrawArg* pDrawArgs, uint32 count) override final;
	void Render(
		uint32 modFrameIndex,
		uint32 backBufferIndex,
		LambdaEngine::CommandList** ppFirstExecutionStage,
		LambdaEngine::CommandList** ppSecondaryExecutionStage,
		bool sleeping) override final;

	static void CreateProjectileMesh(LambdaEngine::Entity entity);

	FORCEINLINE LambdaEngine::FPipelineStageFlag GetFirstPipelineStage() const override final	{ return LambdaEngine::FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }
	FORCEINLINE LambdaEngine::FPipelineStageFlag GetLastPipelineStage() const override final	{ return LambdaEngine::FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }

	FORCEINLINE const LambdaEngine::String& GetName() const override final
	{
		static const LambdaEngine::String name = LambdaEngine::RENDER_GRAPH_PROJECTILES_NAME;
		return name;
	}

private:
	void CreatePipelineLayout();
	bool CreateShaders();
	bool CreateCommonMesh();
	bool CreateDescriptorHeap();
	bool CreateCommandLists(const LambdaEngine::CustomRendererRenderGraphInitDesc* pPreInitDesc);
	bool CreatePipeline();
	// Subscribes to projectile entities
	void SubscribeToProjectiles();

	void OnProjectileCreated(LambdaEngine::Entity entity);
	void OnProjectileRemoval(LambdaEngine::Entity entity);

	void DeleteMarchingCubesGrid(MarchingCubesGrid& marchingCubesGrid);

private:
	LambdaEngine::IDDVector<MarchingCubesGrid> m_MarchingCubesGrids;
	LambdaEngine::IDVector m_ProjectileEntities;

	const LambdaEngine::GraphicsDevice* m_pGraphicsDevice = nullptr;

	LambdaEngine::TSharedRef<LambdaEngine::PipelineLayout> m_PipelineLayout = nullptr;
	LambdaEngine::TSharedRef<LambdaEngine::PipelineState> m_Pipeline = nullptr;
	LambdaEngine::TSharedRef<LambdaEngine::DescriptorHeap> m_DescriptorHeap = nullptr;

	LambdaEngine::CommandAllocator** m_ppComputeCommandAllocators = nullptr;
	LambdaEngine::CommandList** m_ppComputeCommandLists = nullptr;

	GUID_Lambda m_DensityShaderGUID = GUID_NONE;
	GUID_Lambda m_MeshGenShaderGUID = GUID_NONE;
	uint32 m_WorkGroupSize = 0;

	/*	All projectiles share the same mesh to avoid having to recreate an empty vertex buffer each time a
		projectile is created. */
	GUID_Lambda m_MarchingCubesMesh = GUID_NONE;

	// All marching cubes share the same mesh, which has a constant size. This is the size in bytes.
	uint64 m_VertexBufferSize = 0;
};
