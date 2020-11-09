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
		Buffer* pVertexBuffer;
		// pTriangleCountBuffer contains a uint32 that is atomically added when a triangle is added to the mesh
		Buffer* pTriangleCountBuffer;
		// TriangleCount is the CPU-side representation of pTriangleCountBuffer
		uint32 TriangleCount;
		DescriptorSet* pDescriptorSet;
	};

	class ProjectileRenderer : public CustomRenderer
	{
	public:
		ProjectileRenderer(GraphicsDevice* pGraphicsDevice);
		~ProjectileRenderer();

		bool Init() override final;

		bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;
		void Render(
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage,
			bool sleeping) override final;

		void UpdateProjectiles(uint64 modFrameIndex);

		void OnProjectileCreated(Entity entity);
		void OnProjectileRemoval(Entity entity);

		FORCEINLINE FPipelineStageFlag GetFirstPipelineStage() const override final	{ return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }
		FORCEINLINE FPipelineStageFlag GetLastPipelineStage() const override final	{ return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }

		FORCEINLINE const String& GetName() const override final
		{
			static const String name = RENDER_GRAPH_PROJECTILES_NAME;
			return name;
		}

	private:
		void CreatePipelineLayout();
		bool CreateShaders();
		bool CreateDescriptorHeap();
		bool CreateCommandLists(const CustomRendererRenderGraphInitDesc* pPreInitDesc);
		bool CreatePipeline();

		void DeleteMarchingCubesGrid(MarchingCubesGrid& marchingCubesGrid);

	private:
		IDDVector<MarchingCubesGrid> m_MarchingCubesGrids;
		IDVector m_ProjectileEntities;

		const GraphicsDevice* m_pGraphicsDevice = nullptr;

		TSharedRef<PipelineLayout> m_PipelineLayout = nullptr;
		TSharedRef<PipelineState> m_Pipeline = nullptr;
		TSharedRef<DescriptorHeap> m_DescriptorHeap = nullptr;

		CommandAllocator** m_ppComputeCommandAllocators = nullptr;
		CommandList** m_ppComputeCommandLists = nullptr;

		GUID_Lambda m_DensityShaderGUID = GUID_NONE;
		GUID_Lambda m_MeshGenShaderGUID = GUID_NONE;
		uint32 m_WorkGroupSize = 0;
	};
}
